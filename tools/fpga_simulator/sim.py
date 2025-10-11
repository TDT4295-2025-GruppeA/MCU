
import argparse
import struct
import threading
import time

from vpython import box, vector, color, rate, scene, label

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
obs_objs = []  # currently visible objects
staging_objs = []  # objects for next frame
info_label = None
DEBUG = False

def dbg(*args, **kwargs):
    if DEBUG:
        print(*args, **kwargs)


def serial_thread(port, baud):
    global player_pos, obstacles
    if serial is None:
        print("pyserial not installed")
        return
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
        dbg(f"Opened serial {port} @ {baud}")
    except SerialException as e:
        print("Failed to open serial:", e)
        return

    text_buf = b""
    while True:
        try:
            data = ser.read(64)
        except SerialException:
            print("Serial read error, exiting thread")
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
                # Assume only one command per SPI message
                if len(spi_data) == 0:
                    continue
                cmd = spi_data[0]
                if cmd == CMD_BEGIN_UPLOAD:
                    dbg("[SPI] Received: BEGIN_UPLOAD (0xA0)")
                    handle_begin_upload()
                elif cmd == CMD_UPLOAD_TRIANGLE and len(spi_data) >= SIZE_UPLOAD_TRIANGLE:
                    dbg(f"[SPI] Received: UPLOAD_TRIANGLE (0xA1) - data: {spi_data[:SIZE_UPLOAD_TRIANGLE].hex()}")
                    handle_upload_triangle(spi_data[:SIZE_UPLOAD_TRIANGLE])
                elif cmd == CMD_ADD_MODEL_INSTANCE and len(spi_data) >= SIZE_ADD_MODEL_INSTANCE:
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
    dbg("Begin Upload (not used)")

def handle_upload_triangle(packet):
    dbg("Upload Triangle (not used)")

def handle_add_model_instance(packet):
    model_id = packet[2]
    # positions (3 x int32 Q16.16)
    pos = [int.from_bytes(packet[3:7], 'big', signed=True) / 65536.0,
           int.from_bytes(packet[7:11], 'big', signed=True) / 65536.0,
           int.from_bytes(packet[11:15], 'big', signed=True) / 65536.0]

    # rotations (9 x int32 Q16.16)
    rot = [int.from_bytes(packet[15 + i*4:19 + i*4], 'big', signed=True) / 65536.0 for i in range(9)]

    dbg(f"Add Model Instance: id={model_id} pos={pos} rot={rot}")
    if model_id == 0:
        with lock:
            box_obj = box(pos=vector(pos[0], pos[1], pos[2]), size=vector(4, 4, 4), color=color.green, visible=False)
            staging_objs.append(box_obj)

def handle_frame_start():
    dbg("Frame Start")
    with lock:
        staging_objs.clear()

def handle_frame_end():
    dbg("Frame End")
    with lock:
        for ob in obs_objs:
            ob.visible = False
        obs_objs.clear()
        for ob in staging_objs:
            ob.visible = True
            obs_objs.append(ob)
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
    scene.width = 1000
    scene.height = 700
    scene.center = vector(0, 0, 20)
    scene.background = color.gray(0.2)
    info_label = label(pos=vector(-40, 30, 0), text="", height=16, box=False)
    

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
