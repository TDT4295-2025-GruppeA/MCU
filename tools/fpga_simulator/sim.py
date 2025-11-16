# Helper: frame start (clear staging, save shape upload)
def frame_start():
    global current_upload_id, current_upload_tris
    dbg("Frame Start (helper)")
    save_current_upload()
    staging_objs.clear()

# Helper: frame end (render all instances, clear staging)
def frame_end():
    # Debug print: show shape_registry contents before rendering
    if shape_registry:
        msg = f"[SIM] shape_registry contents before render: " + ", ".join([f"id={sid}: {len(tris)} tris" for sid, tris in shape_registry.items()])
    else:
        msg = "[SIM] shape_registry is empty before render."
    # print(msg)
    log_to_file(msg)
    dbg("Frame received")
    global last_frame_time
    current_time = time.time()
    if last_frame_time is not None:
        frame_delta = current_time - last_frame_time
        # dbg(f"[PERF] Frame delta: {frame_delta:.3f}s ({1/frame_delta:.1f} FPS)")
    last_frame_time = current_time
    
    start_time = time.time()
    dbg("Frame End (helper)")
    dbg(f"[DEBUG] Clearing {len(obs_objs)} previous objects")
    for ob in obs_objs:
        try:
            ob.visible = False
        except Exception:
            pass
    obs_objs.clear()
    dbg(f"[DEBUG] Rendering {len(staging_objs)} instances")
    for desc in staging_objs:
        mid = desc.get('id', 0)
        p = desc.get('pos', [0,0,0])
        rot = desc.get('rot', [1,0,0,0,1,0,0,0,1])
        tris = shape_registry.get(mid)
        if USE_CUSTOM_SHAPES:
            msg = f"[SIM] USE_CUSTOM_SHAPES is enabled. shape_id={mid}"
            # print(msg)
            log_to_file(msg)
        else:
            msg = f"[SIM] USE_CUSTOM_SHAPES is disabled. shape_id={mid}"
            # print(msg)
            log_to_file(msg)
        if tris:
            msg = f"[SIM] Rendering custom shape_id={mid} with {len(tris)} triangles."
            # print(msg)
            log_to_file(msg)
            tri_objs = []
            for tri in tris:
                try:
                    tri_obj = create_vpython_triangle(tri, p, rot)
                    tri_objs.append(tri_obj)
                except Exception as e:
                    msg = f"[SIM] Error rendering triangle for shape_id={mid}: {e}"
                    # print(msg)
                    log_to_file(msg)
            obs_objs.extend(tri_objs)
        else:
            msg = f"[SIM] No custom shape found for shape_id={mid}, rendering box instead."
            # print(msg)
            log_to_file(msg)
            try:
                col = color.green if mid == 0 else color.red
                box_obj = create_vpython_box(p, col)
                obs_objs.append(box_obj)
            except Exception as e:
                msg = f"[SIM] Error rendering box for shape_id={mid}: {e}"
                # print(msg)
                log_to_file(msg)
    # print(f"[SIM] Frame render complete. {len(obs_objs)} objects now visible.")
    staging_objs.clear()
    
    render_time = time.time() - start_time
    # msg = f"[SIM] Frame render time: {render_time:.3f}s, Objects: {len(obs_objs)}"
    # print(msg)
    # log_to_file(msg)
import argparse
import struct
import threading
import time
import os

# Q16.16 fixed-point scale
Q16_16_SCALE = 65536.0

from vpython import box, vector, color, rate, scene, label, vertex, triangle
# Toggle: use custom shapes (triangle mesh) or legacy box rendering
USE_CUSTOM_SHAPES = True  # Changed to False for better performance

try:
    import serial
    from serial import SerialException
except Exception:
    serial = None

# SPI protocol command opcodes
CMD_RESET = 0x55
CMD_BEGIN_UPLOAD = 0xA0
CMD_UPLOAD_TRIANGLE = 0xA1
CMD_ADD_INSTANCE = 0xB0
CMD_FRAME_START = 0xF0
CMD_FRAME_END = 0xF1
CMD_POSITION_CAMERA = 0xC0

# SPI protocol packet sizes
SIZE_UPLOAD_TRIANGLE = 43
SIZE_ADD_INSTANCE = 51

# UART markers
UART_PREFIX = b"Sending SPI message: "
UART_SUFFIX = b"SPI message end"




# Shared state
lock = threading.Lock()
# Shape registry: shape_id -> list of triangles (each triangle: [v0, v1, v2])
shape_registry = {}  # dynamic shape definitions
current_upload_id = None
current_upload_tris = []
# Store high-level instance descriptors in staging; actual VPython objects live in obs_objs
obs_objs = []  # currently visible VPython objects
staging_objs = []  # descriptors for next frame: dicts with keys: id, pos, rot
info_label = None
DEBUG = False
last_frame_time = None

# Logging state
LOG_FILE = "sim_debug.log"
LOG_MAX_LINES = 500
log_buffer = []

def log_to_file(msg):
    return True
    global log_buffer
    log_buffer.append(msg)
    if len(log_buffer) > LOG_MAX_LINES:
        log_buffer = log_buffer[-LOG_MAX_LINES:]
    try:
        with open(LOG_FILE, "w") as f:
            f.write("\n".join(log_buffer))
    except Exception:
        pass

def dbg(*args, **kwargs):
    if DEBUG:
        print(*args, **kwargs)


def serial_thread(port, baud):
    global player_pos, obstacles
    if serial is None:
        dbg("pyserial not installed")
        return
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
        dbg(f"Opened serial {port} @ {baud}")
    except SerialException as e:
        dbg("Failed to open serial: %s" % e)
        return

    text_buf = b""
    while True:
        try:
            data = ser.read(64)
        except SerialException:
            dbg("Serial read error, exiting thread")
            break
        if data:
            # dbg(f"[SERIAL] Raw data received: {data.hex()}")
            text_buf += data
            while True:
                start = text_buf.find(UART_PREFIX)
                if start == -1:
                    break
                end = text_buf.find(UART_SUFFIX, start)
                if end == -1:
                    break
                spi_data = text_buf[start + len(UART_PREFIX):end]
                # dbg(f"[SERIAL] SPI message extracted: {spi_data.hex()}")
                text_buf = text_buf[end + len(UART_SUFFIX):]
                # Print SPI message length and hex for every message (debug only)
                dbg(f"[SPI RAW] len={len(spi_data)} hex={spi_data.hex()}")
                if len(spi_data) == 0:
                    continue
                cmd = spi_data[0]
                if cmd == CMD_BEGIN_UPLOAD:
                    dbg("[SPI] Received: BEGIN_UPLOAD (0xA0)")
                    handle_begin_upload(spi_data)
                elif cmd == CMD_UPLOAD_TRIANGLE:
                    dbg(f"[SPI] Received: UPLOAD_TRIANGLE (0xA1) - data: {spi_data[:SIZE_UPLOAD_TRIANGLE].hex()}")
                    handle_upload_triangle(spi_data[:SIZE_UPLOAD_TRIANGLE])
                elif cmd == CMD_ADD_INSTANCE:
                    dbg(f"[SPI] Received: ADD_INSTANCE (0xB0) - data: {spi_data[:SIZE_ADD_INSTANCE].hex()}")
                    handle_add_instance(spi_data[:SIZE_ADD_INSTANCE])
                elif cmd == CMD_FRAME_START:
                    dbg("[SPI] Received: FRAME_START (0xF0)")
                    handle_frame_start()
                elif cmd == CMD_FRAME_END:
                    dbg("[SPI] Received: FRAME_END (0xF1)")
                    handle_frame_end()
                elif cmd == CMD_RESET:
                    dbg("[SPI] Received: RESET (0x00)")
                    handle_reset()
                elif cmd == CMD_POSITION_CAMERA:
                    dbg("[SPI] Received: POSITION_CAMERA (0xC0)")
                    handle_position_camera(spi_data[:SIZE_ADD_INSTANCE])
                else:
                    dbg(f"[SPI] Received: UNKNOWN (0x{cmd:02X})")
# Handler for POSITION_CAMERA message
def handle_position_camera(packet):
    # Packet structure is same as ADD_INSTANCE: [cmd, flag, id, pos(12), rot(36)]
    # We'll use pos for scene.camera.pos and rot for scene.camera.axis
    from vpython import vector, scene
    pos = parse_vertex(packet, 3)
    rot = parse_rotation(packet, 15)
    # rot is a 3x3 matrix flattened; axis is the forward direction (third column)
    # Set camera position (negate y for coordinate system)
    # Set camera axis (forward direction: third column of rotation matrix)
    # Set field of view to 150 degrees in radians
    import math
    scene.fov = math.radians(90)
    axis = vector(rot[2], -rot[5], rot[8]).norm()
    # axis = vector(0,0.25,1).norm()
    scene.camera.axis = axis
    scene.range = 100
    # Set camera up (second column of rotation matrix)
    up = vector(rot[1], -rot[4], rot[7]).norm()
    # up = vector(0,-1,-0.2).norm()
    scene.camera.up = up
    scene.camera.pos = vector(pos[0], -pos[1], -pos[2])
    # Log all relevant matrix columns
    msg = f"[SIM] Set camera pos to {scene.camera.pos}, axis to ({scene.camera.axis.x}, {scene.camera.axis.y}, {scene.camera.axis.z}), up to ({scene.camera.up.x}, {scene.camera.up.y}, {scene.camera.up.z}), matrix: [{rot[0]:.3f} {rot[1]:.3f} {rot[2]:.3f}; {rot[3]:.3f} {rot[4]:.3f} {rot[5]:.3f}; {rot[6]:.3f} {rot[7]:.3f} {rot[8]:.3f}]"
    # print(msg)
    log_to_file(msg)



def handle_upload_triangle(packet):
    global current_upload_tris
    # Parse triangle: 3x (color (2), vertex (12))
    colors = []
    vertices = []
    for i in range(3):
        color_offset = 1 + i * 14
        color_val = int.from_bytes(packet[color_offset:color_offset+2], 'big')
        r = ((color_val >> 10) & 0x1F) / 31.0
        g = ((color_val >> 5) & 0x1F) / 31.0
        b = (color_val & 0x1F) / 31.0
        colors.append((r, g, b))
        vertex_offset = color_offset + 2
        vertex = parse_vertex(packet, vertex_offset)
        vertices.append(vertex)
    current_upload_tris.append((vertices[0], vertices[1], vertices[2], colors))
    msg = f"[SIM] Upload Triangle: v0={vertices[0]}, v1={vertices[1]}, v2={vertices[2]}"
    print(msg)
    log_to_file(msg)



def handle_begin_upload(packet):
    global current_upload_id, current_upload_tris, shape_registry
    save_current_upload()
    # Use model_id from packet[1] (sent by MCU)
    model_id = packet[1]
    current_upload_id = model_id
    current_upload_tris = []
    msg = f"[SIM] Begin Upload: shape_id={current_upload_id}"
    print(msg)
    log_to_file(msg)

def handle_add_instance(packet):
    is_last_model = packet[1] == 0x01
    shape_id = packet[2]  # Sent by MCU, not incremented by sim
    pos = parse_vertex(packet, 3)
    rot = parse_rotation(packet, 15)
    # msg = f"[SIM] Add Instance: id={shape_id} pos={pos} is_last_model={is_last_model}"
    # print(msg)
    # log_to_file(msg)
    # If this is the first instance in a new frame, call frame_start
    if len(staging_objs) == 0:
        frame_start()
    desc = { 'id': shape_id, 'pos': pos, 'rot': rot }
    with lock:
        staging_objs.append(desc)
        if is_last_model:
            # print("Calling frame_end")
            frame_end()

def handle_frame_start():
    global current_upload_id, current_upload_tris
    dbg("Frame Start")
    with lock:
        # If we just finished a shape upload, store it
        save_current_upload()
        # Clear staging descriptors for new frame
        staging_objs.clear()

def handle_frame_end():
    print("Frame received")
    global last_frame_time
    current_time = time.time()
    if last_frame_time is not None:
        frame_delta = current_time - last_frame_time
        print(f"[PERF] Frame delta: {frame_delta:.3f}s ({1/frame_delta:.1f} FPS)")
    last_frame_time = current_time
    
    start_time = time.time()
    dbg("Frame End")
    with lock:
        # Hide and clear existing objects
        for ob in obs_objs:
            try:
                ob.visible = False
            except Exception:
                pass
        obs_objs.clear()


        # Create VPython objects for each descriptor
        for desc in staging_objs:
            mid = desc.get('id', 0)
            p = desc.get('pos', [0,0,0])
            rot = desc.get('rot', [1,0,0,0,1,0,0,0,1])
            tris = shape_registry.get(mid)
            dbg(f"Instance: shape_id={mid}, pos={p}, rot={rot}, tris={'yes' if tris else 'no'}")
            if USE_CUSTOM_SHAPES and tris:
                # Render each triangle as a VPython triangle primitive
                tri_objs = []
                for tri in tris:
                    try:
                        tri_obj = create_vpython_triangle(tri, p)
                        tri_objs.append(tri_obj)
                    except Exception as e:
                        dbg(f"Error rendering triangle for shape_id={mid}: {e}")
                obs_objs.extend(tri_objs)
                dbg(f"Rendered {len(tri_objs)} triangles for shape_id={mid} at pos={p}")
            else:
                # Legacy: draw a box at the shape position
                try:
                    col = color.green if mid == 0 else color.red
                    box_obj = create_vpython_box(p, col)
                    obs_objs.append(box_obj)
                    dbg(f"Rendered box for shape_id={mid} at pos={p}")
                except Exception as e:
                    dbg(f"Error rendering box for shape_id={mid}: {e}")

        # Clear staging descriptors after swapping
        staging_objs.clear()
    
    render_time = time.time() - start_time
    # print(f"[PERF] Frame render time: {render_time:.3f}s, Objects: {len(obs_objs)}")

def handle_reset():
    dbg("Reset")
    with lock:
        for ob in obs_objs:
            if hasattr(ob, 'visible'):
                ob.visible = False
        obs_objs.clear()
        for ob in staging_objs:
            if hasattr(ob, 'visible'):
                ob.visible = False
        staging_objs.clear()
        # Clear all uploaded shapes and uploads in progress
        shape_registry.clear()
        global current_upload_id, current_upload_tris
        current_upload_id = None
        current_upload_tris = []


def create_scene():
    global info_label
    scene.title = "MCU -> FPGA Simulator (visualizer)"
    scene.width = 1200
    scene.height = 900
    scene.center = vector(0, 0, 0) # Same as default values. Player always in (0,0,0)
    scene.forward = vector(0, 0, 1) # Default 0,0,-1 (FPGA assumes positive z-axis in fron of camera)
    scene.up = vector(0,-1,0) # Default 0,1,0 (FPGA/VGA assumes origo (0,0) is top left of screen and positive y-axis pointing down)
    
    scene.autoscale = False
    scene.background = color.gray(0.2)
    info_label = label(pos=vector(-40, 30, 0), text="", height=16, box=False)
    # Ensure object lists are empty at start
    with lock:
        obs_objs.clear()
        staging_objs.clear()
    
def save_current_upload():
    global current_upload_id, current_upload_tris, shape_registry
    if current_upload_id is not None and current_upload_tris:
        msg = f"[SIM] Saving shape_id={current_upload_id} with {len(current_upload_tris)} triangles"
        print(msg)
        log_to_file(msg)
        for idx, tri in enumerate(current_upload_tris):
            v0, v1, v2, colors = tri
            color_str = ', '.join([f"({c[0]:.2f}, {c[1]:.2f}, {c[2]:.2f})" for c in colors])
            msg = f"[SIM]   Triangle {idx} colors: {color_str}"
            print(msg)
            log_to_file(msg)
        shape_registry[current_upload_id] = list(current_upload_tris)
        current_upload_id = None
        current_upload_tris = []
        
# Helper to parse Q16.16 fixed-point from bytes
def parse_q16_16(b):
    return int.from_bytes(b, 'big', signed=True) / Q16_16_SCALE

# Helper to parse a vertex (3x Q16.16)
def parse_vertex(packet, offset):
    return [parse_q16_16(packet[offset + i*4:offset + (i+1)*4]) for i in range(3)]

# Helper to parse a rotation matrix (9x Q16.16)
def parse_rotation(packet, offset):
    return [parse_q16_16(packet[offset + i*4:offset + (i+1)*4]) for i in range(9)]

# Helper to create a VPython triangle from triangle data and position
def apply_rotation(v, rot):
    # v: [x, y, z], rot: 3x3 matrix as flat list
    x = rot[0]*v[0] + rot[1]*v[1] + rot[2]*v[2]
    y = rot[3]*v[0] + rot[4]*v[1] + rot[5]*v[2]
    z = rot[6]*v[0] + rot[7]*v[1] + rot[8]*v[2]
    return [x, y, z]

def create_vpython_triangle(tri, pos, rot):
    from vpython import vertex, vector, triangle
    # tri: (v0, v1, v2, colors)
    v0, v1, v2, colors = tri
    rv0 = apply_rotation(v0, rot)
    rv1 = apply_rotation(v1, rot)
    rv2 = apply_rotation(v2, rot)
    return triangle(
        v0=vertex(pos=vector(*rv0) + vector(*pos), color=vector(*colors[0])),
        v1=vertex(pos=vector(*rv1) + vector(*pos), color=vector(*colors[1])),
        v2=vertex(pos=vector(*rv2) + vector(*pos), color=vector(*colors[2]))
    )

# Helper to create a VPython box from position and color
def create_vpython_box(pos, col, rot):
    from vpython import box, vector
    return box(pos=vector(*pos), size=vector(4,4,4), color=col, visible=True)


# # keyboard handlers

# def keydown(evt):
#     global forward
#     s = evt.key
#     with lock:
#         if s in ('left', 'a'):
#             player_pos['x'] -= 5
#         elif s in ('right', 'd'):
#             player_pos['x'] += 5
#         elif s in ('up', 'w'):
#             player_pos['y'] += 5
#         elif s in ('down', 's'):
#             player_pos['y'] -= 5
#         elif s == ' ':
#             forward = not forward
#         elif s == 'r':
#             player_pos['x'] = 0
#             player_pos['y'] = 0
#             player_pos['z'] = 5


def main():
    parser = argparse.ArgumentParser(description='FPGA visualizer / simulator')
    # parser.add_argument('--mode', choices=['keyboard', 'serial'], default='keyboard', help='Input mode: keyboard or serial')
    parser.add_argument('--serial', help='Serial port to listen for packets')
    parser.add_argument('--baud', type=int, default=115200, help='Serial baudrate')
    parser.add_argument('--debug', action='store_true', help='Enable debug printing')
    args = parser.parse_args()
    global DEBUG
    if getattr(args, 'debug', False):
        DEBUG = True

    create_scene()
    # if args.mode == 'serial' and args.serial:
    t = threading.Thread(target=serial_thread, args=(args.serial, args.baud), daemon=True)
    t.start()
    # scene.bind('keydown', keydown)

    global forward
    frame_count = 0
    last_time = time.time()
    while True:
        rate(30)
        frame_count += 1
        current_time = time.time()
        if frame_count % 60 == 0:  # Every ~0.5 seconds at 120 FPS
            fps = 60 / (current_time - last_time)
            last_time = current_time
            with lock:
                obj_count = len(obs_objs)
            info_label.text = f"FPS: {fps:.1f}, Objects: {obj_count}"
            # print(f"[PERF] FPS: {fps:.1f}, Objects: {obj_count}")


if __name__ == '__main__':
    main()
