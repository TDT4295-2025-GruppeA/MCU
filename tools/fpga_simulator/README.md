FPGA Simulator (Visualiser)
==================

This folder contains the FPGA visualizer / simulator used for local 3D
visualization of what the FPGA expects to render. The simulator is separated
into small modules for packet parsing, serial reading (mirrored UART packets),
and rendering.

Run it with the provided wrapper:

```bash
python3 tools/fpga_simulator/run_fpga_sim.py --serial /dev/ttyUSB0 --debug
```

Install dependencies with:

```bash
python3 -m pip install -r tools/fpga_simulator/requirements.txt
```

Notes:

- The serial reader extracts mirrored SPI packets from UART logs using the
	prefix/suffix markers `Sending SPI message:` and `SPI message end`.

- VPython is used for rendering. If not installed, use:

```bash
python3 -m pip install vpython pyserial
```
