#!/usr/bin/env python3
"""Serial transport and API client. Makes no decisions; pi_logic.py drives it."""

import base64
import os
import requests
import serial
import struct
import time

PORT = '/dev/ttyACM0'
BAUD = 115200
API_URL = os.environ.get('DROUGHTBOT_API_URL', 'http://127.0.0.1:8000').rstrip('/')

# Must match PiFrameHeader in communication_system/src/hub.cpp
MAGIC = 0xA5
FRAME = struct.Struct('<BBBIIIIH')  # magic, kind, node_id, request_id, transfer_id, chunk_index, chunk_count, length

# hub -> pi
KIND_NODE = 0       # response or data from a node
KIND_BUTTON = 1     # button press, request_id is 0
KIND_EVENT = 3      # body: [1 = connected, 0 = disconnected][6 byte MAC]
# pi -> hub
KIND_REQUEST = 2    # [magic][kind][u8 node_id][u32 request_id][u8 command_len][command][2 param bytes]
KIND_LIST = 3       # [magic][kind], hub answers with an event per connected node

ACK_TIMEOUT = 3     # seconds until a node must have answered "ok"
CHUNK_TIMEOUT = 5   # seconds allowed between chunks of one transfer

ser = None
rx_buffer = b''
next_request_id = 1

# (node_id, transfer_id) -> {'request_id': int, 'parts': {chunk_index: bytes}}
active_transfers = {}

# request_id -> {'ctx': dict, 'deadline': float, 'timeout': float}
# ctx['step'] decides how the node's answer is saved.
pending = {}

# node_id -> MAC string, kept up to date from hub events
nodes = {}

# set by pi_logic
on_button = None    # callback(node_id, body: bytes)
on_done = None      # callback(ctx, result, error)
on_node = None      # callback(node_id, mac, connected)

############ Framing ############

def parse_frame(buffer):
    """Return (frame, rest). frame is None until a whole frame is buffered."""
    start = buffer.find(bytes([MAGIC]))
    if start < 0:
        return None, b''
    buffer = buffer[start:]
    
    if len(buffer) < FRAME.size:
        return None, buffer
    
    _, kind, node_id, request_id, transfer_id, chunk_index, chunk_count, length = FRAME.unpack(buffer[:FRAME.size])
    if len(buffer) < FRAME.size + length:
        return None, buffer
    
    frame = {
        'kind': kind,
        'node_id': node_id,
        'request_id': request_id,
        'transfer_id': transfer_id,
        'chunk_index': chunk_index,
        'chunk_count': chunk_count,
        'body': buffer[FRAME.size:FRAME.size + length],
    }
    return frame, buffer[FRAME.size + length:]

# assemble chunks into complete bodies
def process_chunk(frame):
    if frame['chunk_count'] <= 1:
        return frame['body']
    
    # every node counts its own transfer ids
    key = (frame['node_id'], frame['transfer_id'])
    if key not in active_transfers:
        if frame['chunk_index'] != 0:
            return None
        active_transfers[key] = {'request_id': frame['request_id'], 'parts': {}}
    
    parts = active_transfers[key]['parts']
    parts[frame['chunk_index']] = frame['body']
    print(f"chunk {frame['chunk_index'] + 1}/{frame['chunk_count']} ({len(frame['body'])} bytes)")

    # all chunks received?
    if len(parts) == frame['chunk_count']:
        del active_transfers[key]
        assembled = b''.join(parts[i] for i in range(frame['chunk_count']))
        print(f"transfer complete: {len(assembled)} bytes total")
        return assembled
    
    return None

############ API client ############

def api(method, path, json=None):
    url = f'{API_URL}{path}'
    try:
        response = requests.request(method, url, json=json, timeout=30)
    except requests.RequestException as e:
        print(f"{method} {path} failed: {e}")
        return None
    
    if not response.ok:
        print(f"{method} {path} -> {response.status_code}: {response.text}")
        return None
    
    print(f"{method} {path} -> {response.status_code}")
    return response.json() if response.content else {}

def get_plants(serie_id):
    return api('GET', f'/series/{serie_id}/plants') or []

############ Saving answers from nodes ############

def parse_value(body):
    # sendResponseValue answers "ok:<number>"
    return int(body.split(b':', 1)[1])

def save_qr(ctx, body):
    return body.decode('utf-8').strip()

def save_scale(ctx, body):
    start_weight = parse_value(body)
    measurement = api('POST', '/measurements', {
        'plant_id': ctx['plant']['id'],
        'start_weight': start_weight,
        'water_added': None,
    })
    if measurement is None:
        raise RuntimeError("could not save measurement")
    
    ctx['measurement_id'] = measurement['id']
    return start_weight

def save_water(ctx, body):
    water_added = parse_value(body)
    if api('PATCH', f"/measurements/{ctx['measurement_id']}", {'water_added': water_added}) is None:
        raise RuntimeError("could not save water_added")
    
    return water_added

def save_image(ctx, body):
    # pydantic's base64 alphabet is url-safe
    image = api('POST', '/images', {
        'measurement_id': ctx['measurement_id'],
        'image_data': base64.urlsafe_b64encode(body).decode('ascii'),
    })
    if image is None:
        raise RuntimeError("could not save image")
    
    return len(body)

SAVERS = {
    'qr': save_qr,
    'register': save_qr,
    'scale': save_scale,
    'water': save_water,
    'image': save_image,
}

############ Requests and responses ############

def send_request(node_id, command, params=(0, 0), ctx=None, timeout=10):
    """Send a request to a node via the hub and return its id.

    The node has ACK_TIMEOUT seconds to answer "ok", then `timeout` seconds to send
    its result. If either runs out, on_done gets an error.
    """
    global next_request_id
    
    request_id = next_request_id
    next_request_id += 1
    
    cmd = command.encode('ascii')
    frame = struct.pack('<BBBIB', MAGIC, KIND_REQUEST, node_id, request_id, len(cmd)) + cmd + bytes(params)
    
    if ctx is not None:
        pending[request_id] = {
            'ctx': ctx,
            'deadline': time.monotonic() + ACK_TIMEOUT,
            'timeout': timeout,
        }
    
    print(f"Request {request_id} to node {node_id}: {command} {tuple(params)}")
    ser.write(frame)
    return request_id

def request_node_list():
    ser.write(struct.pack('<BB', MAGIC, KIND_LIST))

def fail_request(request_id, error):
    entry = pending.pop(request_id)
    
    # drop half received transfers of this request
    for key in [k for k, t in active_transfers.items() if t['request_id'] == request_id]:
        del active_transfers[key]
    
    on_done(entry['ctx'], None, error)

def check_timeouts():
    now = time.monotonic()
    for request_id in [r for r, e in pending.items() if e['deadline'] < now]:
        fail_request(request_id, 'timeout')

def handle_node_message(request_id, body):
    entry = pending.get(request_id)
    if entry is None:
        print(f"Message for untracked request {request_id}")
        return
    
    # the node acks right away, the real answer comes later under the same id
    if body == b'ok':
        entry['deadline'] = time.monotonic() + entry['timeout']
        return
    
    del pending[request_id]
    ctx = entry['ctx']
    
    if body.startswith(b'error:'):
        on_done(ctx, None, body.decode('utf-8', 'replace'))
        return
    
    try:
        result = SAVERS[ctx['step']](ctx, body)
    except (ValueError, IndexError, UnicodeDecodeError, RuntimeError) as e:
        on_done(ctx, None, f"{ctx['step']}: {e}")
        return
    
    on_done(ctx, result, None)

def handle_event(node_id, body):
    if len(body) != 7:
        return
    
    connected = body[0] == 1
    mac = ':'.join(f'{b:02X}' for b in body[1:])
    if connected:
        nodes[node_id] = mac
    else:
        nodes.pop(node_id, None)
    
    print(f"Node {node_id} {mac} {'connected' if connected else 'disconnected'}")
    on_node(node_id, mac, connected)

def handle_frame(frame):
    if frame['kind'] == KIND_EVENT:
        handle_event(frame['node_id'], frame['body'])
        return
    
    # a transfer in progress is alive, give it more time
    entry = pending.get(frame['request_id'])
    if entry is not None and frame['chunk_count'] > 1:
        entry['deadline'] = time.monotonic() + CHUNK_TIMEOUT
    
    body = process_chunk(frame)
    if body is None:
        return
    
    if frame['kind'] == KIND_BUTTON:
        on_button(frame['node_id'], body)
    elif frame['kind'] == KIND_NODE:
        handle_node_message(frame['request_id'], body)
    else:
        print(f"Unknown frame kind {frame['kind']}")

def poll():
    """Read whatever the hub sent, handle every complete frame, fail expired requests."""
    global rx_buffer
    
    if ser.in_waiting:
        rx_buffer += ser.read(ser.in_waiting)
    
    while True:
        frame, rx_buffer = parse_frame(rx_buffer)
        if frame is None:
            break
        handle_frame(frame)
    
    check_timeouts()

def run(port=PORT):
    global ser
    ser = serial.Serial(port, BAUD, timeout=1)
    print(f"Connected to {port}")
    request_node_list()
    
    try:
        while True:
            poll()
            time.sleep(0.01)
    except KeyboardInterrupt:
        print("Done")
    finally:
        ser.close()
