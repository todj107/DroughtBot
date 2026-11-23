#!/usr/bin/env python3
"""Decides what to do: reacts to buttons and node answers, sends the requests.
Saving data is done by pi_serial_handler.py."""

import os

import pi_serial_handler as handler

SERIE_ID = int(os.environ.get('DROUGHTBOT_SERIE_ID', '1'))

# Which physical node does what, by MAC (printed by the node and the hub on connect).
# Node ids come from the hub in connection order, so they are looked up through the MAC.
ROLE_MACS = {
    'station': os.environ.get('DROUGHTBOT_STATION_MAC', '').upper(),   # QR scanner, scale, pump
    'camera': os.environ.get('DROUGHTBOT_CAMERA_MAC', '').upper(),
}

# button body first byte
BUTTON_START = 0
BUTTON_ABORT = 1

# the node reads a single byte of water amount
MAX_WATER = 255

# seconds a node gets to deliver its result after its "ok"
TIMEOUTS = {
    'qr': 30,
    'scale': 15,
    'water': 120,
    'image': 30,
}

plants = []

def node_for(role):
    for node_id, mac in handler.nodes.items():
        if mac == ROLE_MACS[role]:
            return node_id
    return None

def plant_for_qr(qr_code):
    for plant in plants:
        if plant['qr_code'] == qr_code:
            return plant
    return None

def request(ctx, role, step, command, params=(0, 0)):
    node_id = node_for(role)
    if node_id is None:
        finish(ctx, f"{role} node is not connected")
        return
    
    ctx['step'] = step
    handler.send_request(node_id, command, params, ctx=ctx, timeout=TIMEOUTS[step])

def start_cycle():
    request({}, 'station', 'qr', 'read_qr_code')

def finish(ctx, reason):
    print(f"Cycle finished: {reason}")

def on_node(node_id, mac, connected):
    pass

def on_button(node_id, body):
    if not body:
        return
    
    if body[0] == BUTTON_START:
        start_cycle()
    elif body[0] == BUTTON_ABORT:
        station = node_for('station')
        if station is not None:
            handler.send_request(station, 'abort')
    else:
        print(f"Unknown button {body[0]}")

def on_done(ctx, result, error):
    step = ctx['step']
    
    if error:
        
        # never leave the pump running if we lost track of it
        if step == 'water':
            station = node_for('station')
            if station is not None:
                handler.send_request(station, 'abort')
        
        finish(ctx, f"{step} failed: {error}")
        return
    
    if step == 'qr':
        plant = plant_for_qr(result)
        if plant is None:
            finish(ctx, f"no plant with qr code {result!r}")
            return
        ctx['plant'] = plant
        request(ctx, 'station', 'scale', 'read_scale')
    
    elif step == 'scale':
        water = round(ctx['plant']['target_weight'] - result)
        if water <= 0:
            finish(ctx, "plant already at target weight")
            return
        request(ctx, 'station', 'water', 'water_process', (min(water, MAX_WATER), 0))
    
    elif step == 'water':
        request(ctx, 'camera', 'image', 'send_image')
    
    elif step == 'image':
        finish(ctx, "done")

if __name__ == '__main__':
    plants = handler.get_plants(SERIE_ID)
    print(f"Loaded {len(plants)} plants from series {SERIE_ID}")
    
    handler.on_button = on_button
    handler.on_done = on_done
    handler.on_node = on_node
    handler.run()
