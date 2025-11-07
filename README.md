# Flyboy 3D - STM32-FPGA Graphics Game

## Project Overview

A real-time 3D endless runner game using STM32U5 microcontroller for game logic and FPGA for graphics rendering. The player navigates through 3D space avoiding obstacles, with persistent high scores and configurable shapes stored on SD card.

### Team Structure

- **MCU Programming** (2 people): Game logic, input handling, SD card storage, testing framework
- **FPGA Development** (3 people): 3D graphics rendering, display output
- **PCB Design** (2 people): Hardware integration, SD card interface

## Current Status

### Completed

- **Game Engine**: Endless runner with collision detection
- **SD Card Storage**: Persistent high scores and shape data
- **SPI Communication**: Dual SPI (FPGA graphics + SD card)
- **Input System**: ADC-based dual button control
- **Shape System**: Dynamic 3D shapes (player, cubes, cones) with SD persistence
- **Collision Detection**: AABB (Axis-Aligned Bounding Box) system
- **Testing Framework**: Unit tests
- **Debug Interface**: UART command system for runtime testing

### In Progress

- Transformations of shapes sent to FPGA
- Difficulty progression system
- Sound effects integration

## Features

### Game Mechanics
- Forward auto-scrolling at configurable speed
- Left/right movement to avoid obstacles
- Dynamic obstacle spawning with minimum spacing
- Score tracking (distance + obstacles passed)
- Collision detection with boundary checking

### Data Persistence
- High score saving to SD card
- Game statistics (total games, play time)
- 3D shape storage and loading
- Automatic save on game over

### Testing System
- Unit tests for all modules
- SD card verification tests
- UART command interface for runtime testing
- See [Test Documentation](test_documentation.md) for details

## Hardware Setup

### Board Configuration

```
MCU: STM32U545RE-Q (Nucleo-64)
Core: ARM Cortex-M33 @ 4 MHz
Memory: 512KB Flash, 256KB SRAM
Debug: ST-LINK V3 integrated
```

### Pin Assignments

```
SPI1 (FPGA Communication):
- PA5 (D13) - SCK
- PA6 (D12) - MISO  
- PA7 (D11) - MOSI
- PA4 (D10) - CS (GPIO)

SPI3 (SD Card):
- PC10 - SCK
- PC11 - MISO (needs 10kΩ pull-up to 3.3V)
- PC12 - MOSI
- PB0  - CS (GPIO)

ADC Inputs:
- PC0 - Button 1 (ADC1_IN1)
- PC1 - Button 2 (ADC1_IN2)

UART1 (Debug):
- PA9  - TX (via ST-LINK)
- PA10 - RX (via ST-LINK)
- 115200 baud, 8N1
```

## Software Architecture

### Modular Structure

```
Core/
├── Inc/
│   ├── main.h
│   ├── peripherals.h       # Hardware initialization
│   ├── uart_debug.h        # Debug output
│   ├── adc_functions.h     # ADC helpers
│   ├── Game/
│   │   ├── game.h          # Main game logic
│   │   ├── collision.h     # Collision detection
│   │   ├── obstacles.h     # Obstacle management
│   │   ├── shapes.h        # 3D shape definitions
│   │   ├── input.h         # Input processing
│   │   └── spi_protocol.h  # FPGA communication
│   ├── SDCard/
│   │   ├── sd_card.h       # SD card driver
│   │   └── game_storage.h  # Save/load system
│   └── Test/
│       ├── test_framework.h # Testing macros
│       └── command_handler.h # UART commands
├── Src/
│   ├── main.c              # Entry point (150 lines)
│   ├── peripherals.c       # All peripheral init
│   ├── uart_debug.c        # UART functions
│   ├── Game/               # Game implementation
│   ├── SDCard/             # Storage implementation
│   └── Test/               # Test suites
```

### Communication Protocols

#### FPGA Graphics Protocol (SPI1)

```c
// Shape definition packet
CMD_DEFINE_SHAPE (0x01)
[cmd | shape_id | vertex_count | vertices... | triangle_count | triangles...]

// Instance rendering packet  
CMD_ADD_INSTANCE (0x02)
[cmd | shape_id | x | y | z | rotation]

// Frame control
CMD_BEGIN_RENDER (0x04)
[cmd | frame_number]
```

#### SD Card Storage Format

```
Block 100: Game save data (high score, stats)
Block 200: Player shape
Block 201: Cube shape  
Block 202: Cone shape
Block 500+: Test data
```

## Building and Running

### Build Configurations

1. **Debug** (Normal Game)
   - Project → Properties → C/C++ Build → Settings
   - Configuration: Debug
   - Build and run normally

2. **UNIT_TESTS** (Test Mode)
   - Add define: `RUN_UNIT_TESTS`
   - Runs all tests on startup
   - See [Test Documentation](test_documentation.md)

### Quick Start

```bash
# Clone repository
git clone [repository-url]

# Open in STM32CubeIDE
File → Import → Existing Projects

# Build (Ctrl+B)
# Flash to board (F11)

# Connect serial terminal
sudo screen /dev/ttyACM0 115200
```

## Playing the Game

### Controls

**ADC Buttons:**
- Button 1: Move left
- Button 2: Move right
- Both: Pause/Resume
- Hold Button 1: Reset game


### Gameplay

1. Game starts automatically on power-up
2. Player moves forward continuously
3. Use buttons to dodge obstacles
4. Score increases with distance and obstacles passed
5. Game saves high score automatically

## Development Guide

### Adding New Features

#### New Obstacle Types
```c
// In obstacles.c
void Spawn_NewObstacleType(void) {
    obstacle->type = OBSTACLE_MOVING;
    obstacle->velocity = 5.0f;
}
```

#### New Shapes
```c
// In shapes.c
void Shapes_CreatePyramid(Shape3D* shape) {
    shape->vertex_count = 5;
    // Define vertices and triangles
}
```

### Running Tests

```bash
# Via build configuration change preprocessor
Set RUN_UNIT_TESTS define → Build → Run

```

See [Test Documentation](test_documentation.md) for complete testing guide.

## Performance

- Game loop: 50 Hz (20ms update)
- SPI1 (FPGA): 250 kHz
- SPI3 (SD): 156.25 kHz  
- Position updates: Real-time
- Save time: <100ms

## Troubleshooting

### SD Card Not Working
- Ensure 10kΩ pull-up on MISO (PC11)
- Check both ground connections (pins 3 & 6)
- Try different card (≤32GB, FAT32 formatted)
- Run `test sd` command

### No Button Response
- Verify pull-ups on PC0/PC1
- Buttons should read <100 when pressed

### Game Not Starting
- Check UART output for initialization
- Verify all shapes loaded from SD
- Try `reset` command

## Testing

The project includes comprehensive unit tests covering:
- Collision detection algorithms
- Obstacle spawning logic
- SD card operations
- Shape storage/loading
- Game state management

**[See Test Documentation](test_documentation.md)** for:
- Test framework setup
- Running tests
- Writing new tests
- Coverage reports

## Resources

### Documentation
- [STM32U5 Reference Manual](https://www.st.com/resource/en/user_manual/um3062-stm32u3u5-nucleo64-board-mb1841-stmicroelectronics.pdf)
- [SD Card SPI Protocol](http://elm-chan.org/docs/mmc/mmc_e.html)

### Tools
- [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html)
- [PuTTY](https://www.putty.org/) - Windows terminal
- [Logic Analyzer](https://www.saleae.com/) - SPI debugging

## Contributing

### Before Committing
- [ ] Run all unit tests (Change preprocessor to run)
- [ ] Verify SD card operations
- [ ] Check no compiler warnings
- [ ] Update documentation for new features
- [ ] Test on actual hardware


## Contact

- **MCU/Game Logic**: Jorgnik@ntnu.no
- **FPGA Development**: Jorgfje@ntnu.no  
- **PCB Design**: Rikkees@ntnu.no

---

*Last Updated: November 2025*  
*Version: 1.0.0*
