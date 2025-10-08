
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
SIZE_ADD_MODEL_INSTANCE = 44

# UART markers
UART_PREFIX = b"Sending SPI message: "
UART_SUFFIX = b"SPI message end"


# Shared state
lock = threading.Lock()
obs_objs = []  # currently visible objects
staging_objs = []  # objects for next frame
info_label = None


def serial_thread(port, baud):
    global player_pos, obstacles
    if serial is None:
        print("pyserial not installed")
        return
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
        print(f"Opened serial {port} @ {baud}")
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
            text_buf += data
            while True:
                start = text_buf.find(UART_PREFIX)
                if start == -1:
                    break
                end = text_buf.find(UART_SUFFIX, start)
                if end == -1:
                    break
                spi_data = text_buf[start + len(UART_PREFIX):end]
                text_buf = text_buf[end + len(UART_SUFFIX):]
                while len(spi_data) > 0:
                    cmd = spi_data[0]
                    if cmd == CMD_BEGIN_UPLOAD:
                        handle_begin_upload()
                        spi_data = spi_data[1:]
                    elif cmd == CMD_UPLOAD_TRIANGLE and len(spi_data) >= SIZE_UPLOAD_TRIANGLE:
                        handle_upload_triangle(spi_data[:SIZE_UPLOAD_TRIANGLE])
                        spi_data = spi_data[SIZE_UPLOAD_TRIANGLE:]
                    elif cmd == CMD_ADD_MODEL_INSTANCE and len(spi_data) >= SIZE_ADD_MODEL_INSTANCE:
                        handle_add_model_instance(spi_data[:SIZE_ADD_MODEL_INSTANCE])
                        spi_data = spi_data[SIZE_ADD_MODEL_INSTANCE:]
                    elif cmd == CMD_FRAME_START:
                        handle_frame_start()
                        spi_data = spi_data[1:]
                    elif cmd == CMD_FRAME_END:
                        handle_frame_end()
                        spi_data = spi_data[1:]
                    elif cmd == CMD_RESET:
                        handle_reset()
                        spi_data = spi_data[1:]
                    else:
                        spi_data = spi_data[1:]

def handle_begin_upload():
    print("Begin Upload (not used)")

def handle_upload_triangle(packet):
    print("Upload Triangle (not used)")

def handle_add_model_instance(packet):
    model_id = packet[2]
    pos = [int.from_bytes(packet[3:7], 'big', signed=True) / 65536.0,
           int.from_bytes(packet[7:11], 'big', signed=True) / 65536.0,
           int.from_bytes(packet[11:15], 'big', signed=True) / 65536.0]
    rot = [int.from_bytes(packet[15 + i*4:19 + i*4], 'big', signed=True) / 65536.0 for i in range(9)]
    print(f"Add Model Instance: id={model_id} pos={pos} rot={rot}")
    if model_id == 0:
        with lock:
            box_obj = box(pos=vector(pos[0], pos[1], pos[2]), size=vector(4, 4, 4), color=color.green, visible=False)
            staging_objs.append(box_obj)

def handle_frame_start():
    print("Frame Start")
    with lock:
        staging_objs.clear()

def handle_frame_end():
    print("Frame End")
    with lock:
        for ob in obs_objs:
            ob.visible = False
        obs_objs.clear()
        for ob in staging_objs:
            ob.visible = True
            obs_objs.append(ob)
        staging_objs.clear()

def handle_reset():
    print("Reset")
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
    args = parser.parse_args()

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
