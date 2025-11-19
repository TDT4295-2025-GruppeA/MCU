#!/usr/bin/env python3

"""Run the FPGA visualizer / simulator.

Use `--serial` to connect to a device

Example:
    python3 tools/fpga_simulator/run_fpga_sim.py --serial /dev/ttyUSB0 --debug
"""

import argparse

from vpython import rate

from modules.renderer import Renderer
from modules.serial_reader import SerialReader
from modules.simulator import Simulator, handle_command

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--serial", help="Serial port to listen")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--debug", action="store_true")
    args = parser.parse_args()

    renderer = Renderer(debug=args.debug)
    renderer.create_scene()
    sim = Simulator(renderer=renderer, debug=args.debug)

    def on_packet(spi_data):
        handle_command(sim, spi_data)

    reader = SerialReader(
        port=args.serial, baud=args.baud, callback=on_packet, debug=args.debug
    )
    reader.start()

    try:
        while True:
            rate(30)
    except KeyboardInterrupt:
        reader.stop()
        renderer.stop()
