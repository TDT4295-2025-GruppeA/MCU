# STM32-FPGA 3D Graphics Controller

## Project Overview

This project implements a real-time 3D graphics control system using an STM32U5 microcontroller as the game logic processor and an FPGA as the graphics rendering engine. The system is designed for a 7-person team developing a complete gaming platform with custom PCB and FPGA acceleration.

### Team Structure

- **MCU Programming** (2 person): Game logic, controller input, SPI communication
- **FPGA Development** (3 people): 3D graphics rendering, display output
- **PCB Design** (2 people): Hardware integration

### Goal

Create a simple 3D game where the player moves through a 3D space, with the STM32 handling game logic and sending coordinate data to the FPGA for rendering.
The first databus should send the different game objects (cubes, the play character/object etc). Then the rest of the messages should specify the location of different objects to be spawned.
We start with only moving the player in the x-axis so we fly straigth forward and spawning cubes infront to avoid.

## Current Status

### Completed

- **SPI Communication**: Working bidirectional SPI interface at configurable speeds
- **Button Input**: Single button control with gesture recognition (single/double click, hold)
- **Position Tracking**: Real-time 3D coordinate system (X, Y, Z)
- **UART Debug Interface**: Serial communication for testing and debugging
- **Modular Code Structure**: Separated game logic from hardware abstraction

### 🚧 In Progress

- FPGA protocol finalization
- Multiple button support
- Game physics implementation

### TODO

- Connect to actual FPGA hardware
- Optimize rendering pipeline

## Hardware Setup

### Board

- **MCU**: STM32U545RE-Q (Nucleo-64)
- **Core**: ARM Cortex-M33
- **Clock**: 4 MHz (configurable up to 160 MHz)
- **Debug**: ST-LINK V3 integrated

### Pin Configuration

``` markdown
SPI1 (FPGA Communication):
- PA5 (D13) - SCK  (Clock)
- PA6 (D12) - MISO (Master In)
- PA7 (D11) - MOSI (Master Out)  
- PA4 (D10) - CS   (Chip Select - GPIO)

UART1 (Debug Console):
- Connected via ST-LINK VCP
- 115200 baud, 8N1

User Input:
- PC13 - USER button (blue)
- PA5  - LED_GREEN
```

## Software Architecture

### File Structure

``` markdown
Core/
├── Inc/
	├──Game/
		├── collision.h 
		├── game_types.h
		├── game.h 	 
		├── input.h
		├── obstacles.h
		├── shapes.h
		├── spi_protocol.h
│   ├── main.h           # System definitions
│   ├── buttons.h        # Button input handling
│   └── fpga_spi.h       # FPGA communication protocol
├── Src/
	├──Game/
		├── collision.c
		├── game.c
		├── input.c
		├── obstacles.c
		├── shapes.c
		├── spi_protocol.c
│   ├── main.c           # System initialization
│   ├── game.c           # Game logic implementation
│   ├── buttons.c        # Button driver
│   └── fpga_spi.c       # SPI protocol implementation
```

### Communication Protocol

#### Position Update Packet (MCU → FPGA)

```c
struct {
    uint8_t cmd;     // 0x01 = POSITION_UPDATE
    int16_t x;       // X coordinate (-100 to +100)
    int16_t y;       // Y coordinate (-50 to +50)
    int16_t z;       // Z coordinate (0 to ∞)
} packet;  // 7 bytes total
```

#### Example SPI Transaction

```markdown
[Frame 1] POS: X=0, Y=0, Z=3
SPI->[01 0000 0000 0003]

[Frame 2] POS: X=-20, Y=0, Z=6
SPI->[01 FFEC 0000 0006]
```

## Building and Running

### Prerequisites

- STM32CubeIDE 1.16.0 or later
- ST-LINK drivers
- Serial terminal (PuTTY, Tera Term, or screen)

### Build Instructions

1. Clone the repository
2. Open project in STM32CubeIDE
3. Build project (Ctrl+B)
4. Flash to board (F11)

### Testing Without FPGA

1. **Loopback Test**: Connect PA7 (MOSI) to PA6 (MISO) with jumper wire
2. **Serial Monitor**: Connect to COM port at 115200 baud (sudo minicom -D /dev/ttyACM0 -b 115200)
3. **Run Tests**: Press user button or send commands via UART

### UART Commands

``` markdown
Movement:
  a/d - Move left/right
  w/s - Start/stop forward movement
  r   - Reset position

Debug:
  0-3 - Set debug verbosity
  p   - Toggle SPI data display
  ?   - Show help
```

### Button Controls

- **Single Click**: Move left (-20 units)
- **Double Click**: Move right (+20 units)
- **Hold (>1s)**: Toggle forward movement

## Development Guide

### Adding New Features

#### 1. New Game Objects

Add to `game.h`:

```c
typedef struct {
    Position pos;
    uint8_t type;
    Color color;
} GameObject;
```

#### 2. New SPI Commands

Add to `fpga_spi.h`:

```c
#define CMD_DRAW_CUBE   0x10
#define CMD_DRAW_SPHERE 0x11
```

#### 3. New Input Methods

Modify `buttons.c` for additional buttons or implement I2C for external controllers.

### Debugging Tips

1. **Check SPI signals** with oscilloscope on PA5 (SCK) and PA7 (MOSI)
2. **Monitor UART output** for position updates and debug messages
3. **Use LED feedback** - Green LED indicates button press/SPI activity
4. **Enable verbose mode** - Send '3' via UART for detailed output

### Code Style

- Use clear variable names
- Comment protocol packets
- Keep functions under 50 lines
- Use integer math where possible (avoid float)

## FPGA Integration

### Expected FPGA Functionality

1. Receive position data via SPI
2. Apply 3D transformation matrices
3. Render objects at given coordinates
4. Handle display output

### Coordinate System

- **Origin**: Center of screen at Z=0
- **X-axis**: -100 (left) to +100 (right)
- **Y-axis**: -50 (bottom) to +50 (top)
- **Z-axis**: 0 (near) to ∞ (far)

### Timing Requirements

- Position updates: Every 1.5 seconds (configurable)
- SPI clock: 250 kHz (4MHz/16 prescaler)
- Game loop: 50 Hz polling rate

## Troubleshooting

### No UART Output

- Check COM port in Device Manager
- Verify 115200 baud rate
- Ensure USART1 is enabled in CubeMX

### SPI Not Working

- Verify loopback connection (PA7→PA6)
- Check SPI1 enabled in CubeMX
- Monitor CS pin (PA4) - should pulse low during transmission

### Position Values Not Printing

- Code uses integer casting: `UART_Printf("X=%d", (int)pos.x)`
- To enable float support: Add `-u _printf_float` to linker flags (adds ~10KB)

## Contributing

### Testing Checklist

- [ ] Code compiles without warnings
- [ ] SPI loopback test passes
- [ ] Button inputs respond correctly
- [ ] UART output is readable
- [ ] No memory leaks (check with debugger)

## Resources

### Documentation

- [STM32U5 Reference Manual](https://www.st.com/resource/en/reference_manual/rm0456-stm32u5-series-armbased-32bit-mcus-stmicroelectronics.pdf)
- [HAL Driver User Manual](https://www.st.com/resource/en/user_manual/um2570-description-of-stm32u5-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
- [Nucleo-U545RE User Manual](https://www.st.com/resource/en/user_manual/um2861-stm32u5-nucleo64-boards-mb1549-stmicroelectronics.pdf)

### Tools

- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) - Configuration tool
- [PuTTY](https://www.putty.org/) - Serial terminal
- [Logic Analyzer Software](https://www.saleae.com/downloads/) - For debugging SPI

## License

This project is developed as part of a university course. All rights reserved.

## Contact

For questions about:

- **MCU/Game Logic**: [Jorgnik@ntnu.no]
- **FPGA Development**: [Jorgfje@ntnu.no]
- **PCB Design**: [Rikkees@ntnu.no]

---

*Last Updated: September 2025*
*Version: 0.1.0*
