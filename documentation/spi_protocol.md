# SPI Communication Protocol Specification

## 1. Overview

This document describes the SPI protocol used for communication between the MCU and FPGA. The protocol supports model upload, scene management, and frame rendering control.

## 2. Physical Layer

- **Interface:** SPI
- **Mode:** Mode 0 (CLKPolarity=0, CLKPhase=0)
- **Bit Order:** MSB first
- **Data Width:** 8 bits per transfer

## 3. Message Structure

All messages are composed of a command byte followed by zero or more data bytes. Multi-byte fields are transmitted in big-endian order.

| Byte Index | Description         |
|------------|--------------------|
| 0          | Command (uint8)    |
| 1..N       | Data (optional)    |

## 4. Command Reference

| Command Name      | Opcode | Description                                 | Request Format                  | Response Format                |
|-------------------|--------|---------------------------------------------|---------------------------------|--------------------------------|
| Reset             | 0x00   | Reset all registers to initial state        | [0x00]                         | None                           |
| Begin Upload      | 0xA0   | Start model upload sequence                 | [0xA0]                         | [Object ID (uint8)]            |
| Upload Triangle   | 0xA1   | Upload one triangle to current model        | [0xA1, Color (2), V0 (12), V1 (12), V2 (12)] | None |
| Add Model Instance| 0xB0   | Add model instance to scene                 | [0xB0, Model ID, Transform]     | None                           |
| Mark Frame Start  | 0xF0   | Indicate start of frame rendering           | [0xF0]                         | None                           |
| Mark Frame End    | 0xF1   | Indicate end of frame rendering             | [0xF1]                         | None                           |

### Command Sizes

| Command Name        | Total Size (bytes) | Field Sizes (bytes) |
|---------------------|-------------------|---------------------|
| Reset               | 1                 | Command: 1          |
| Begin Upload        | 1                 | Command: 1          |
| Upload Triangle     | 41                | Command: 1, Color: 2, Vertex 0: 12, Vertex 1: 12, Vertex 2: 12 |
| Add Model Instance  | 51                | Command: 1, Reserved: 1, Model ID: 1, Position X/Y/Z: 4×3=12, Rotation XX/XY/XZ/YX/YY/YZ/ZX/ZY/ZZ: 4×9=36 |
| Mark Frame Start    | 1                 | Command: 1          |
| Mark Frame End      | 1                 | Command: 1          |

### Field Definitions

- **Color:** 2 bytes (format: 5 bits R, 5 bits G, 5 bits B, 1 bit reserved)
- **Vertex (V0, V1, V2):** 12 bytes each (3 x signed 32-bit fixed-point, Q16.16 format)
- **Model ID:** 1 byte
- **Position (X, Y, Z):** 4 bytes each (signed 32-bit fixed-point, Q16.16 format)
- **Rotation (XX, XY, XZ, YX, YY, YZ, ZX, ZY, ZZ):** 4 bytes each (signed 32-bit fixed-point, Q16.16 format)

**Fixed-point format:**
All vertex, position, and rotation fields use signed 32-bit fixed-point representation (Q16.16 format), with 16 bits for the integer part and 16 bits for the fractional part. Values are transmitted in big-endian byte order.

## 5. Communication Sequence

### Model Upload Example

1. MCU sends `Begin Upload` (`0xA0`)
2. FPGA responds with `Object ID`
3. MCU sends multiple `Upload Triangle` (`0xA1, ...`)

### Frame Rendering Example

1. MCU sends `Mark Frame Start` (`0xF0`)
2. MCU sends `Add Model Instance` (`0xB0, ...`)
3. MCU sends `Mark Frame End` (`0xF1`)

## 6. Error Handling

## 7. Versioning

- **Protocol Version:** 1.0
- Changes are indicated by updating the version field in documentation and firmware.
