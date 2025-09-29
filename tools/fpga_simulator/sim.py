"""
Simple FPGA simulator and local keyboard-based game visualizer.

Usage:
  - Local (keyboard) mode: python sim.py
  - Serial mode: python sim.py --serial [device] --baud 115200

Requirements:
  * vpython 
  * [pyserial]

This script supports two modes:
 - Local mode: you can drive the player with arrow keys and space to toggle forward movement.
 - Serial mode: it reads binary PositionPacket frames from a serial port and visualizes them.

Packet format expected (little-endian):
  uint8 cmd (0x01 = POSITION_UPDATE)
  int16 x
  int16 y
  int16 z
Total size = 7 bytes
"""

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

PACKET_FMT = "<Bhhh"  # uint8, int16, int16, int16
PACKET_SIZE = struct.calcsize(PACKET_FMT)
CMD_POSITION = 0x01

# Shared state
player_pos = {'x': 0, 'y': 0, 'z': 5}
obstacles = []  # list of dicts with x,y,z
lock = threading.Lock()
forward = False
speed_z = 0.2  # units per frame when forward

# Visual objects
player_obj = None
obs_objs = []
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

    buf = bytearray()
    while True:
        try:
            data = ser.read(64)
        except SerialException:
            print("Serial read error, exiting thread")
            break
        if data:
            buf.extend(data)
            # parse frames
            while len(buf) >= PACKET_SIZE:
                pkt = buf[:PACKET_SIZE]
                buf = buf[PACKET_SIZE:]
                try:
                    cmd, x, y, z = struct.unpack(PACKET_FMT, pkt)
                except struct.error:
                    continue
                if cmd == CMD_POSITION:
                    with lock:
                        player_pos['x'] = x
                        player_pos['y'] = y
                        player_pos['z'] = z
        else:
            time.sleep(0.005)


def create_scene():
    global player_obj, obs_objs, info_label
    scene.title = "MCU -> FPGA Simulator (visualizer)"
    scene.width = 1000
    scene.height = 700
    scene.center = vector(0, 0, 20)
    scene.background = color.gray(0.2)

    player_obj = box(pos=vector(0, 0, 0), size=vector(4, 2, 2), color=color.cyan)

    for i in range(10):
        ob = box(pos=vector((i % 5) * 12 - 24, 0, i * 10 + 30), size=vector(6, 6, 6), color=color.red)
        obs_objs.append(ob)

    info_label = label(pos=vector(-40, 30, 0), text="", height=16, box=False)


def update_visuals():
    # update player
    with lock:
        px = player_pos['x']
        py = player_pos['y']
        pz = player_pos['z']
    player_obj.pos = vector(px / 1.0, py / 1.0, pz / 1.0)
    # obstacles are static
    info_label.text = f"Player: x={px} y={py} z={pz}  Forward={'ON' if forward else 'OFF'}"


# keyboard handlers

def keydown(evt):
    global forward
    s = evt.key
    with lock:
        if s in ('left', 'a'):
            player_pos['x'] -= 5
        elif s in ('right', 'd'):
            player_pos['x'] += 5
        elif s in ('up', 'w'):
            player_pos['y'] += 5
        elif s in ('down', 's'):
            player_pos['y'] -= 5
        elif s == ' ':
            forward = not forward
        elif s == 'r':
            player_pos['x'] = 0
            player_pos['y'] = 0
            player_pos['z'] = 5


def main():
    parser = argparse.ArgumentParser(description='Simple FPGA visualizer / simulator')
    parser.add_argument('--serial', help='Serial port to listen for packets')
    parser.add_argument('--baud', type=int, default=115200, help='Serial baudrate')
    args = parser.parse_args()

    if args.serial:
        t = threading.Thread(target=serial_thread, args=(args.serial, args.baud), daemon=True)
        t.start()

    create_scene()
    scene.bind('keydown', keydown)

    global forward
    while True:
        rate(60)
        if forward:
            with lock:
                player_pos['z'] += speed_z * 10
        update_visuals()


if __name__ == '__main__':
    main()
