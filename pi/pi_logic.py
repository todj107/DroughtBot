#!/usr/bin/env python3
"""Decides what to do: reacts to buttons and node answers, sends the requests.
Saving data is done by pi_serial_handler.py."""

import pi_serial_handler as handler

_pending = None  # {'ctx': ctx, 'resume': callable} — set while waiting for confirmation

# series ID will be set dynamically based on the QR code of the plant.
SERIE_ID = None

# station node MAC set based on the QR code of the plant.
STATION_MAC = None

# camera node MAC set based on the QR code of the plant.
CAMERA_MAC = None

# inout node MAC set based on the QR code of the plant.
INOUT_MAC = None

ROLE_MACS = {}

# button body first byte
BUTTON_START = 2
BUTTON_ABORT = 1

# ReadyCode values — must match espnow_protocol.h ReadyCode enum
START_PROCESS  = 1   # "Place the plant on the scale and press OK when ready"
START_WATERING = 2   # "Put the pump-handle inside the soil and press OK when ready"
START_IMAGE    = 3   # "Remove the pump-handle and press OK when ready to take image"

# the node reads a single byte of water amount
MAX_WATER = 255

# seconds a node gets to deliver its result after its "ok"
TIMEOUTS = {
    'qr': 60,
    'scale': 120,
    'water': 120,
    'image': 240,
}

def load_node_macs():
    ROLE_MACS.clear()
    for node in handler.api('GET', '/nodes') or []:
        role = node.get('roles', '').lower()
        mac = node.get('mac', '').upper()
        if role and mac:
            ROLE_MACS[role] = mac
    print(f"Node MACs loaded: {ROLE_MACS}")

def node_for(role):
    for node_id, mac in handler.nodes.items():
        if mac == ROLE_MACS[role]:
            return node_id
    return None

def request(ctx, role, step, command, params=(0, 0)):
    node_id = node_for(role)
    if node_id is None:
        finish(ctx, f"{role} node is not connected")
        return
    
    ctx['step'] = step
    handler.send_request(node_id, command, params, ctx=ctx, timeout=TIMEOUTS[step])

def confirm(ctx, ready_code, resume):
    """Send a ReadyCode to the inout node display and wait for button press."""
    global _pending
    inout = node_for('inout')
    if inout is not None:
        handler.send_request(inout, 'display', (ready_code, 0))
    _pending = {'ctx': ctx, 'resume': resume}

def start_cycle():
    request({}, 'station', 'qr', 'read_qr_code')

def finish(ctx, reason):
    print(f"Cycle finished: {reason}")

def on_node(node_id, mac, connected):
    pass

def on_button(node_id, body):
    global _pending
    if not body:
        return

    if body[0] == BUTTON_START:
        if _pending is not None:
            resume = _pending['resume']
            _pending = None
            resume()
        else:
            load_node_macs()
            start_cycle()

    elif body[0] == BUTTON_ABORT:
        _pending = None
        station = node_for('station')
        if station is not None:
            handler.send_request(station, 'abort')
        load_node_macs()
        start_cycle()
    else:
        print(f"Unknown button {body[0]}")

def on_done(ctx, result, error):
    global _pending
    step = ctx['step']

    if error:
        _pending = None
        # never leave the pump running if we lost track of it
        if step == 'water':
            station = node_for('station')
            if station is not None:
                handler.send_request(station, 'abort')
        finish(ctx, f"{step} failed: {error}")
        return

    if step == 'qr':
        plant = handler.api('GET', f'/plants/qr/{result}')
        if plant is None:
            finish(ctx, f"no plant with qr code {result!r}")
            return
        ctx['plant'] = plant
        confirm(ctx, START_PROCESS, lambda: request(ctx, 'station', 'scale', 'read_scale'))

    elif step == 'scale':
        water = round(ctx['plant']['target_weight'] - result)
        if water <= 0:
            finish(ctx, "plant already at target weight")
            return
        ctx['water_amount'] = min(water, MAX_WATER)
        confirm(ctx, START_WATERING,
                lambda: request(ctx, 'station', 'water', 'water_process', (ctx['water_amount'], 0)))

    elif step == 'water':
        confirm(ctx, START_IMAGE, lambda: request(ctx, 'camera', 'image', 'send_image'))

    elif step == 'image':
        finish(ctx, "done")

if __name__ == '__main__':
    load_node_macs()
    handler.on_button = on_button
    handler.on_done = on_done
    handler.on_node = on_node
    handler.run()
