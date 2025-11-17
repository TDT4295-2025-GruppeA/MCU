# serial_reader.py
#
# Handles reading from serial. The project's test/firmware code mirrors SPI
# messages by printing them as UART lines; `SerialReader` extracts those SPI
# messages from UART logs using the `UART_PREFIX` and `UART_SUFFIX` markers.

import threading
import time

import serial
from serial import SerialException

from .packets import UART_PREFIX, UART_SUFFIX


class SerialReader:
    def __init__(self, port=None, baud=115200, callback=None, debug=False):
        self.port = port
        self.baud = baud
        self.callback = callback
        self.debug = debug
        self._stop = False
        self._thread = None

    def start(self):
        self._stop = False
        self._thread = threading.Thread(target=self._run, daemon=True)
        self._thread.start()

    def stop(self):
        self._stop = True
        if self._thread is not None:
            self._thread.join()

    def _log(self, *args):
        if self.debug:
            print(*args)

    def _run(self):
        if serial is None or self.port is None:
            self._log("Serial not available or no port specified")
            return

        try:
            ser = serial.Serial(self.port, self.baud, timeout=0.1)
        except SerialException as e:
            self._log("Serial open failed:", e)
            return

        buf = b""
        while not self._stop:
            try:
                data = ser.read(64)
            except SerialException as e:
                self._log("Serial read failed:", e)
                break
            if not data:
                time.sleep(0.01)
                continue

            buf += data
            # Extract mirrored SPI packets using UART_PREFIX/ SUFFIX
            while True:
                start = buf.find(UART_PREFIX)
                if start == -1:
                    break
                end = buf.find(UART_SUFFIX, start)
                if end == -1:
                    break
                spi_bytes = buf[start + len(UART_PREFIX) : end]
                buf = buf[end + len(UART_SUFFIX) :]
                if self.debug:
                    self._log("SPI mirror found, len=", len(spi_bytes))
                if self.callback:
                    try:
                        self.callback(spi_bytes)
                    except Exception as e:
                        self._log("Callback raised", e)

    # parse from an existing byte stream / log file (useful for testing)
    def feed_from_bytes(self, bdata: bytes):
        # Extract packets from bdata
        start = 0
        while True:
            s = bdata.find(UART_PREFIX, start)
            if s == -1:
                return
            e = bdata.find(UART_SUFFIX, s)
            if e == -1:
                return
            spi_bytes = bdata[s + len(UART_PREFIX) : e]
            start = e + len(UART_SUFFIX)
            if self.callback:
                self.callback(spi_bytes)
