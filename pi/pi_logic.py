#!/usr/bin/env python3
"""Decides what to do: reacts to buttons and node answers, sends the requests.
Saving data is done by pi_serial_handler.py.

The brain is always in one of three states:
  idle            - not _busy and _pending is None, OK starts a cycle
  waiting for node - _busy, OK is ignored until the result arrives
  waiting for OK   - _pending is set, OK runs the next step
"""

import functools
import time
import traceback
import pi_serial_handler as handler

_pending = None  # {'ctx': ctx, 'resume': callable} — set while waiting for confirmation
_busy = False    # True while waiting for a node to deliver its result
_cycle = 0       # raised on every start/abort so late results can be ignored
_retry = None    # (due time, cycle, action) — a busy node to ask again

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
BUTTON_OTHER = 3
BUTTON_START = 2
BUTTON_ABORT = 1

# values must match the enums in espnow_protocol.h
STATUS_ERROR   = 1
STATUS_RUNNING = 2
STATUS_READY   = 3

START_PROCESS  = 1   # "Place the plant on the scale and press OK when ready"
START_WATERING = 2   # "Put the pump-handle inside the soil and press OK when ready"
START_IMAGE    = 3   # "Remove the pump-handle and press OK when ready to take image"
NEXT_PLANT     = 4   # "QR saved. Show the next plant and press OK"
SERIES_DONE    = 5   # "All plants in the series are registered"
RETRY_QR       = 6   # "QR not saved. Check the plant and press OK to retry"
CYCLE_DONE     = 7   # "Done. Press OK to start the next plant"
ENOUGH_WATER   = 8   # "Plant needs no water. Press OK for the next plant"

# ErrorCode
NODE_BUSY          = 4
ABORTED_PROCESS    = 5
PLANT_NOT_FOUND    = 11
NODE_NOT_CONNECTED = 12
NODE_TIMEOUT       = 13
SYSTEM_ERROR       = 14
NO_SERIES          = 15

# step -> RunningCode
RUNNING_CODES = {'qr': 2, 'scale': 2, 'water': 1, 'image': 3, 'register': 4}

# the node reads a single byte of water amount
MAX_WATER = 255

# seconds a node gets to deliver its result after its "ok"
TIMEOUTS = {
    'qr': 60,
    'scale': 80,
    'water': 240,
    'image': 240,
    'register': 60,
}

RETRY_DELAY = 3    # seconds before asking a busy node again
MAX_RETRIES = 5    # give up on a busy node after this many retries

def load_node_macs():
    nodes = handler.api('GET', '/nodes')
    if nodes is None:
        print("Could not load node MACs, keeping the old ones")
        return
    ROLE_MACS.clear()
    for node in nodes:
        role = (node.get('roles') or '').lower()
        mac = (node.get('mac') or '').upper()
        if role and mac:
            ROLE_MACS[role] = mac
    print(f"Node MACs loaded: {ROLE_MACS}")

def node_for(role):
    mac = ROLE_MACS.get(role)
    for node_id, node_mac in handler.nodes.items():
        if node_mac == mac:
            return node_id
    return None

def show(status, code):
    """Update the inout node display."""
    inout = node_for('inout')
    if inout is not None:
        handler.send_request(inout, 'display', (status, code))

def abort_station():
    station = node_for('station')
    if station is not None:
        handler.send_request(station, 'abort')

def _safe(callback):
    """Log errors from a callback and go back to idle instead of stopping the script."""
    @functools.wraps(callback)
    def wrapper(*args):
        global _cycle
        try:
            callback(*args)
        except Exception:
            traceback.print_exc()
            _cycle += 1
            try:
                abort_station()
                finish(f"error in {callback.__name__}", STATUS_ERROR, SYSTEM_ERROR)
            except Exception:
                traceback.print_exc()
    return wrapper

def request(ctx, role, step, command, params=(0, 0)):
    global _busy
    node_id = node_for(role)
    if node_id is None:
        finish(f"{role} node is not connected", STATUS_ERROR, NODE_NOT_CONNECTED)
        return

    ctx['step'] = step
    ctx['role'] = role
    ctx['command'] = command
    ctx['params'] = params
    _busy = True
    show(STATUS_RUNNING, RUNNING_CODES[step])
    handler.send_request(node_id, command, params, ctx=ctx, timeout=TIMEOUTS[step])

def confirm(ctx, ready_code, resume):
    """Send a ReadyCode to the inout node display and wait for button press."""
    global _pending, _busy
    if node_for('inout') is None:
        finish("inout node is not connected")
        return

    _busy = False
    _pending = {'ctx': ctx, 'resume': resume}
    show(STATUS_READY, ready_code)

### Database calls for qr code - series reading
def get_latest_series():
    return handler.api('GET', '/series/latest')

def get_series_plants(serie_id):
    return handler.api('GET', f'/series/{serie_id}/plants')

def save_qr_code(plant_id, qr_code):
    return handler.api('PATCH', f'/plants/{plant_id}', {'qr_code': qr_code})  # None = failed
  
def start_reading_series():
    """Show series id and plant count, then wait for OK."""
    global _pending, _cycle
    load_node_macs()
    inout = node_for('inout')
    if inout is None:
        finish("inout node is not connected")
        return

    series = get_latest_series()
    if not series:
        finish("No series found", STATUS_ERROR, NO_SERIES)
        return
    
    plants = get_series_plants(series['id'])
    if not plants:
        finish("No plants in series found", STATUS_ERROR, NO_SERIES)
        return

    # each value is sent to the node as one byte
    if series['id'] > 255 or len(plants) > 255:
        finish("series id or plant count does not fit in one byte [NEED TO FIX THIS]",
               STATUS_ERROR, SYSTEM_ERROR)
        return

    _cycle += 1
    ctx = {'cycle': _cycle, 'plants': sorted(plants, key=lambda p: p['id']), 'index': 0, 'seen': set()}
    _pending = {'ctx': ctx, 'resume': lambda: read_plant_qr(ctx)}
    handler.send_request(inout, 'series_info', (series['id'], len(plants)))

def read_plant_qr(ctx):
    print(f"Reading QR for plant {ctx['index'] + 1} of {len(ctx['plants'])}")
    request(ctx, 'station', 'register', 'read_qr_code')

def register_qr(ctx, qr_code):
    """Save the scanned code on the current plant, then ask for the next one."""
    retry = lambda: read_plant_qr(ctx)

    if qr_code in ctx['seen']:
        print(f"QR {qr_code!r} was already used in this series")
        confirm(ctx, RETRY_QR, retry)
        return

    plant = ctx['plants'][ctx['index']]
    if save_qr_code(plant['id'], qr_code) is None:
        print(f"Could not save QR {qr_code!r} for plant {plant['id']}")
        confirm(ctx, RETRY_QR, retry)
        return

    ctx['seen'].add(qr_code)
    ctx['index'] += 1
    if ctx['index'] < len(ctx['plants']):
        confirm(ctx, NEXT_PLANT, retry)
    else:
        finish("series registered", STATUS_READY, SERIES_DONE)

def start_cycle():
    global _pending, _cycle
    _cycle += 1
    _pending = None
    request({'cycle': _cycle}, 'station', 'qr', 'read_qr_code')

def finish(reason, status=None, code=None):
    """Go back to idle and, if given, tell the user why on the display."""
    global _pending, _busy, _retry
    _pending = None
    _busy = False
    _retry = None
    print(f"Cycle finished: {reason}")
    if status is not None:
        show(status, code)

def on_node(node_id, mac, connected):
    pass

@_safe
def on_tick():
    """Called from the handler's main loop; sends a retry when it is due."""
    global _retry
    if _retry is None or time.monotonic() < _retry[0]:
        return
    _, cycle, action = _retry
    _retry = None
    if cycle == _cycle:
        action()

@_safe
def on_button(node_id, body):
    global _pending, _busy, _cycle, _retry
    if not body:
        return

    if body[0] == BUTTON_START:
        if _busy:
            print("OK ignored, still waiting for a node")
        elif _pending is not None:
            resume = _pending['resume']
            _pending = None
            resume()
        else:
            load_node_macs()
            start_cycle()

    elif body[0] == BUTTON_ABORT:
        _cycle += 1      # anything still on its way now belongs to an old cycle
        _pending = None
        _busy = False    # idle: the next OK starts a new cycle
        _retry = None
        abort_station()
        show(STATUS_ERROR, ABORTED_PROCESS)

    elif body[0] == BUTTON_OTHER:
        if not _busy and _pending is None:
            start_reading_series()
    else:
        print(f"Unknown button {body[0]}")

def error_code(error):
    """Translate an error from the handler to an ErrorCode for the display."""
    code = str(error).partition(':')[2]
    if code.isdigit():
        return int(code)
    if error == 'timeout':
        return NODE_TIMEOUT
    if error in ('error:no_node', 'error:hub_send_failed'):
        return NODE_NOT_CONNECTED
    return SYSTEM_ERROR

@_safe
def on_done(ctx, result, error):
    global _pending, _busy, _retry
    if not ctx or ctx.get('cycle') != _cycle:
        print(f"Ignored result from an old cycle: {result!r} {error!r}")
        return
    step = ctx['step']

    # try again later if the node is busy (we stay _busy meanwhile)
    if error == f'error:{NODE_BUSY}' and step in ('water', 'image', 'register'):
        ctx['retries'] = ctx.get('retries', 0) + 1
        if ctx['retries'] <= MAX_RETRIES:
            _retry = (time.monotonic() + RETRY_DELAY, _cycle,
                      lambda: request(ctx, ctx['role'], step, ctx['command'], ctx['params']))
            return

    ctx['retries'] = 0

    if error:
        # never leave the pump running if we lost track of it
        if step == 'water':
            abort_station()
        if step == 'register':
            # keep the progress: OK scans the same plant again
            show(STATUS_ERROR, error_code(error))
            _busy = False
            _pending = {'ctx': ctx, 'resume': lambda: read_plant_qr(ctx)}
            print(f"register failed: {error}, press OK to retry")
            return
        finish(f"{step} failed: {error}", STATUS_ERROR, error_code(error))
        return

    if step == 'register':
        register_qr(ctx, result)

    elif step == 'qr':
        plant = handler.api('GET', f'/plants/qr/{result}')
        if plant is None:
            finish(f"no plant with qr code {result!r}", STATUS_ERROR, PLANT_NOT_FOUND)
            return
        ctx['plant'] = plant
        confirm(ctx, START_PROCESS, lambda: request(ctx, 'station', 'scale', 'read_scale'))

    elif step == 'scale':
        water = round(ctx['plant']['target_weight'] - result)
        if water <= 0:
            finish("plant already at target weight", STATUS_READY, ENOUGH_WATER)
            return
        ctx['water_amount'] = min(water, MAX_WATER)
        confirm(ctx, START_WATERING,
                lambda: request(ctx, 'station', 'water', 'water_process', (ctx['water_amount'], 0)))

    elif step == 'water':
        confirm(ctx, START_IMAGE, lambda: request(ctx, 'camera', 'image', 'send_image'))

    elif step == 'image':
        finish("done", STATUS_READY, CYCLE_DONE)

if __name__ == '__main__':
    load_node_macs()
    handler.on_button = on_button
    handler.on_done = on_done
    handler.on_node = on_node
    handler.on_tick = on_tick
    handler.run()