# packets.py
# Parsing helpers and shared constants for the simulator

Q16_16_SCALE = 65536.0

# SPI opcodes -- keep these public so other modules import them
CMD_RESET = 0x55
CMD_BEGIN_UPLOAD = 0xA0
CMD_UPLOAD_TRIANGLE = 0xA1
CMD_ADD_INSTANCE = 0xB0
CMD_FRAME_START = 0xF0
CMD_FRAME_END = 0xF1
CMD_POSITION_CAMERA = 0xC0

# expected sizes
SIZE_UPLOAD_TRIANGLE = 43
SIZE_ADD_INSTANCE = 51

UART_PREFIX = b"Sending SPI message: "
UART_SUFFIX = b"SPI message end"


def parse_q16_16(b: bytes) -> float:
    """Parse a Q16.16 signed fixed-point 4-byte value to float."""
    return int.from_bytes(b, "big", signed=True) / Q16_16_SCALE


def parse_vertex(packet: bytes, offset: int) -> list[float]:
    """Parse three Q16.16 values from packet[offset:offset+12]"""
    return [
        parse_q16_16(packet[offset + i * 4 : offset + (i + 1) * 4])
        for i in range(3)
    ]


def parse_rotation(packet: bytes, offset: int) -> list[float]:
    """Parse nine Q16.16 values for a 3x3 rotation matrix"""
    return [
        parse_q16_16(packet[offset + i * 4 : offset + (i + 1) * 4])
        for i in range(9)
    ]


# helper to check opcode
def opcode_from_packet(packet: bytes) -> int:
    return packet[0] if packet else -1
