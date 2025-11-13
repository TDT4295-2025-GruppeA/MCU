# Helper: frame start (clear staging, save shape upload)
def frame_start():
    global current_upload_id, current_upload_tris
    dbg("Frame Start (helper)")
    save_current_upload()
    staging_objs.clear()

# Helper: frame end (render all instances, clear staging)
def frame_end():
    dbg("Frame received")
    global last_frame_time
    current_time = time.time()
    if last_frame_time is not None:
        frame_delta = current_time - last_frame_time
        dbg(f"[PERF] Frame delta: {frame_delta:.3f}s ({1/frame_delta:.1f} FPS)")
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
        dbg(f"[DEBUG] Instance: shape_id={mid}, pos={p}, rot={rot}, tris={'yes' if tris else 'no'}")
        if USE_CUSTOM_SHAPES and tris:
            tri_objs = []
            for idx, tri in enumerate(tris):
                v0, v1, v2, tri_color = tri
                dbg(f"[DEBUG]   Triangle {idx}: v0={v0}, v1={v1}, v2={v2}, color={tri_color}, instance_pos={p}")
                try:
                    tri_obj = create_vpython_triangle(tri, p)
                    tri_objs.append(tri_obj)
                except Exception as e:
                    dbg(f"Error rendering triangle for shape_id={mid}: {e}")
            obs_objs.extend(tri_objs)
            dbg(f"[DEBUG] Rendered {len(tri_objs)} triangles for shape_id={mid} at pos={p}")
        else:
            try:
                col = color.green if mid == 0 else color.red
                box_obj = create_vpython_box(p, col)
                obs_objs.append(box_obj)
                dbg(f"[DEBUG] Rendered box for shape_id={mid} at pos={p}")
            except Exception as e:
                dbg(f"Error rendering box for shape_id={mid}: {e}")
    dbg(f"[DEBUG] Frame render complete. {len(obs_objs)} objects now visible.")
    staging_objs.clear()
    
    render_time = time.time() - start_time
    dbg(f"[PERF] Frame render time: {render_time:.3f}s, Objects: {len(obs_objs)}")
import argparse
import struct
import threading
import time

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
CMD_RESET = 0x00
CMD_BEGIN_UPLOAD = 0xA0
CMD_UPLOAD_TRIANGLE = 0xA1
CMD_ADD_INSTANCE = 0xB0
CMD_FRAME_START = 0xF0
CMD_FRAME_END = 0xF1

# SPI protocol packet sizes
SIZE_UPLOAD_TRIANGLE = 39
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
                else:
                    dbg(f"[SPI] Received: UNKNOWN (0x{cmd:02X})")



def handle_upload_triangle(packet):
    global current_upload_tris
    # Parse triangle: color (2), v0 (12), v1 (12), v2 (12)
    color_val = int.from_bytes(packet[1:3], 'big')
    r = ((color_val >> 10) & 0x1F) / 31.0
    g = ((color_val >> 5) & 0x1F) / 31.0
    b = (color_val & 0x1F) / 31.0
    tri_color = (r, g, b)
    v0 = parse_vertex(packet, 3)
    v1 = parse_vertex(packet, 15)
    v2 = parse_vertex(packet, 27)
    current_upload_tris.append((v0, v1, v2, tri_color))
    dbg(f"Upload Triangle: v0={v0}, v1={v1}, v2={v2}, color={tri_color}")



def handle_begin_upload(packet):
    global current_upload_id, current_upload_tris, shape_registry
    save_current_upload()
    # Use model_id from packet[1] (sent by MCU)
    model_id = packet[1]
    current_upload_id = model_id
    current_upload_tris = []
    dbg(f"Begin Upload: shape_id={current_upload_id}")

def handle_add_instance(packet):
    is_last_model = packet[1] == 0x01
    shape_id = packet[2]  # Sent by MCU, not incremented by sim
    pos = parse_vertex(packet, 3)
    rot = parse_rotation(packet, 15)
    dbg(f"Add Instance: id={shape_id} pos={pos} rot={rot} is_last_model={is_last_model}")
    # If this is the first instance in a new frame, call frame_start
    if len(staging_objs) == 0:
        frame_start()
    desc = { 'id': shape_id, 'pos': pos, 'rot': rot }
    with lock:
        staging_objs.append(desc)
        if is_last_model:
            print("Calling frame_end")
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
    print(f"[PERF] Frame render time: {render_time:.3f}s, Objects: {len(obs_objs)}")

def handle_reset():
    dbg("Reset")
    with lock:
        for ob in obs_objs:
            ob.visible = False
        obs_objs.clear()
        for ob in staging_objs:
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
    scene.center = vector(0, 2, 0)
    scene.forward = vector(0, -0.25, -1)
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
        print(f"[DEBUG] Saving shape_id={current_upload_id} with {len(current_upload_tris)} triangles")
        for idx, tri in enumerate(current_upload_tris):
            print(f"[DEBUG]   Triangle {idx}: v0={tri[0]}, v1={tri[1]}, v2={tri[2]}")
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
def create_vpython_triangle(tri, pos):
    from vpython import vertex, vector, triangle
    # tri: (v0, v1, v2, tri_color)
    v0, v1, v2, tri_color = tri
    return triangle(
        v0=vertex(pos=vector(*v0) + vector(*pos), color=vector(*tri_color)),
        v1=vertex(pos=vector(*v1) + vector(*pos), color=vector(*tri_color)),
        v2=vertex(pos=vector(*v2) + vector(*pos), color=vector(*tri_color))
    )

# Helper to create a VPython box from position and color
def create_vpython_box(pos, col):
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
    parser.add_argument('--baud', type=int, default=230400, help='Serial baudrate')
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
        rate(120)
        frame_count += 1
        current_time = time.time()
        if frame_count % 60 == 0:  # Every ~0.5 seconds at 120 FPS
            fps = 60 / (current_time - last_time)
            last_time = current_time
            with lock:
                obj_count = len(obs_objs)
            info_label.text = f"FPS: {fps:.1f}, Objects: {obj_count}"
            print(f"[PERF] FPS: {fps:.1f}, Objects: {obj_count}")


if __name__ == '__main__':
    main()
