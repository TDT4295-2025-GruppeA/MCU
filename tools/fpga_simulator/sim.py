
import argparse
import struct
import threading
import time

from vpython import box, vector, color, rate, scene, label, vertex, triangle
# Toggle: use custom shapes (triangle mesh) or legacy box rendering
USE_CUSTOM_SHAPES = True

try:
    import serial
    from serial import SerialException
except Exception:
    serial = None

# SPI protocol command opcodes
CMD_RESET = 0x00
CMD_BEGIN_UPLOAD = 0xA0
CMD_UPLOAD_TRIANGLE = 0xA1
CMD_ADD_MODEL_INSTANCE = 0xB0
CMD_FRAME_START = 0xF0
CMD_FRAME_END = 0xF1

# SPI protocol packet sizes
SIZE_UPLOAD_TRIANGLE = 41
SIZE_ADD_MODEL_INSTANCE = 51

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
                    handle_begin_upload()
                elif cmd == CMD_UPLOAD_TRIANGLE:
                    dbg(f"[SPI] Received: UPLOAD_TRIANGLE (0xA1) - data: {spi_data[:SIZE_UPLOAD_TRIANGLE].hex()}")
                    handle_upload_triangle(spi_data[:SIZE_UPLOAD_TRIANGLE])
                elif cmd == CMD_ADD_MODEL_INSTANCE:
                    dbg(f"[SPI] Received: ADD_MODEL_INSTANCE (0xB0) - data: {spi_data[:SIZE_ADD_MODEL_INSTANCE].hex()}")
                    handle_add_model_instance(spi_data[:SIZE_ADD_MODEL_INSTANCE])
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

def handle_begin_upload():
    global current_upload_id, current_upload_tris, shape_registry
    # If there is an unfinished upload, save it before starting a new one
    if current_upload_id is not None and current_upload_tris:
        dbg(f"[DEBUG] BeginUpload: Saving previous shape_id={current_upload_id} with {len(current_upload_tris)} triangles before new upload")
        shape_registry[current_upload_id] = list(current_upload_tris)
    # Assign next available shape ID
    current_upload_id = len(shape_registry)
    current_upload_tris = []
    dbg(f"Begin Upload: shape_id={current_upload_id}")

def handle_upload_triangle(packet):
    global current_upload_tris
    # Parse triangle: color (2), v0 (12), v1 (12), v2 (12)
    # We ignore color for now
    v0 = [int.from_bytes(packet[3:7], 'big', signed=True) / 65536.0,
        int.from_bytes(packet[7:11], 'big', signed=True) / 65536.0,
        int.from_bytes(packet[11:15], 'big', signed=True) / 65536.0]
    v1 = [int.from_bytes(packet[15:19], 'big', signed=True) / 65536.0,
        int.from_bytes(packet[19:23], 'big', signed=True) / 65536.0,
        int.from_bytes(packet[23:27], 'big', signed=True) / 65536.0]
    v2 = [int.from_bytes(packet[27:31], 'big', signed=True) / 65536.0,
        int.from_bytes(packet[31:35], 'big', signed=True) / 65536.0,
        int.from_bytes(packet[35:39], 'big', signed=True) / 65536.0]
    current_upload_tris.append((v0, v1, v2))
    dbg(f"Upload Triangle: v0={v0}, v1={v1}, v2={v2}")

def handle_add_model_instance(packet):
    model_id = packet[2]
    # positions (3 x int32 Q16.16)
    pos = [int.from_bytes(packet[3:7], 'big', signed=True) / 65536.0,
           int.from_bytes(packet[7:11], 'big', signed=True) / 65536.0,
           int.from_bytes(packet[11:15], 'big', signed=True) / 65536.0]

    # rotations (9 x int32 Q16.16)
    rot = [int.from_bytes(packet[15 + i*4:19 + i*4], 'big', signed=True) / 65536.0 for i in range(9)]

    dbg(f"Add Model Instance: id={model_id} pos={pos} rot={rot}")
    # Append a descriptor to staging; actual VPython objects are created on frame end in the main thread
    desc = { 'id': model_id, 'pos': pos, 'rot': rot }
    with lock:
        staging_objs.append(desc)

def handle_frame_start():
    global current_upload_id, current_upload_tris
    dbg("Frame Start")
    with lock:
        # If we just finished a shape upload, store it
        if current_upload_id is not None and current_upload_tris:
            shape_registry[current_upload_id] = list(current_upload_tris)
            dbg(f"Registered shape_id={current_upload_id} with {len(current_upload_tris)} triangles")
            for idx, tri in enumerate(current_upload_tris):
                dbg(f"  Triangle {idx}: v0={tri[0]}, v1={tri[1]}, v2={tri[2]}")
            current_upload_id = None
            current_upload_tris = []
        # Clear staging descriptors for new frame
        staging_objs.clear()

def handle_frame_end():
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
                        tri_obj = triangle(
                            v0=vertex(pos=vector(*tri[0]) + vector(p[0], p[1], p[2]), color=color.white),
                            v1=vertex(pos=vector(*tri[1]) + vector(p[0], p[1], p[2]), color=color.white),
                            v2=vertex(pos=vector(*tri[2]) + vector(p[0], p[1], p[2]), color=color.white)
                        )
                        tri_objs.append(tri_obj)
                    except Exception as e:
                        dbg(f"Error rendering triangle for shape_id={mid}: {e}")
                obs_objs.extend(tri_objs)
                dbg(f"Rendered {len(tri_objs)} triangles for shape_id={mid} at pos={p}")
            else:
                # Legacy: draw a box at the shape position
                try:
                    col = color.green if mid == 0 else color.red
                    box_obj = box(pos=vector(p[0], p[1], p[2]), size=vector(4,4,4), color=col, visible=True)
                    obs_objs.append(box_obj)
                    dbg(f"Rendered box for shape_id={mid} at pos={p}")
                except Exception as e:
                    dbg(f"Error rendering box for shape_id={mid}: {e}")

        # Clear staging descriptors after swapping
        staging_objs.clear()

def handle_reset():
    dbg("Reset")
    with lock:
        for ob in obs_objs:
            ob.visible = False
        obs_objs.clear()
        for ob in staging_objs:
            ob.visible = False
        staging_objs.clear()


def create_scene():
    global info_label
    scene.title = "MCU -> FPGA Simulator (visualizer)"
    scene.width = 1200
    scene.height = 900
    scene.center = vector(0, 8, 12)
    scene.forward = vector(0, -0.25, -1)
    scene.autoscale = False
    scene.background = color.gray(0.2)
    info_label = label(pos=vector(-40, 30, 0), text="", height=16, box=False)
    # Ensure object lists are empty at start
    with lock:
        obs_objs.clear()
        staging_objs.clear()
    

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
    while True:
        rate(60)


if __name__ == '__main__':
    main()
