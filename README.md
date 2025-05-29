# Real-Time Sensor Data Logger

A comprehensive C-based sensor data logging system designed for embedded systems, IoT applications, industrial monitoring, and civil engineering instrumentation.

## Features

- **Dual Mode Operation**: Simulated sensors (no hardware required) and hardware sensor support
- **Real-time Data Logging**: Timestamped data logging to CSV files
- **Statistical Analysis**: Mean, min/max, moving averages, rate of change calculations
- **Anomaly Detection**: Threshold-based alerts and trend analysis
- **Modular Design**: Clean separation of concerns with dedicated modules
- **Bridge Vibration Monitoring**: Example application for structural health monitoring

## Project Structure

```
├── src/
│   ├── main.c              # Main application entry point
│   ├── sensor_simulator.c  # Simulated sensor data generation
│   ├── hardware_interface.c # Hardware sensor communication
│   ├── data_logger.c       # Data logging and CSV management
│   ├── data_analyzer.c     # Statistical analysis and anomaly detection
│   └── utils.c             # Utility functions (timing, formatting)
├── include/
│   ├── sensor_simulator.h
│   ├── hardware_interface.h
│   ├── data_logger.h
│   ├── data_analyzer.h
│   └── utils.h
├── data/                   # Generated CSV log files
├── Makefile               # Build configuration
└── README.md              # This file
```

## Building the Project

```bash
make clean
make
```

## Usage

### Simulated Mode (Default)
```bash
./datalogger
```

### Hardware Mode
```bash
./datalogger --hardware /dev/ttyUSB0
```

### Configuration Options
- `--duration <seconds>`: Set logging duration (default: 60 seconds)
- `--interval <ms>`: Set sampling interval in milliseconds (default: 100ms)
- `--output <filename>`: Set output CSV filename
- `--threshold <value>`: Set anomaly detection threshold

## Example Applications

1. **Bridge Vibration Monitor**: Monitor structural vibrations with accelerometer data
2. **Environmental Logger**: Track temperature, humidity, and air quality
3. **Industrial Equipment Monitor**: Monitor machinery vibration and temperature
4. **Structural Health Monitoring**: Long-term monitoring of building integrity

## Dependencies

- Standard C library
- POSIX threads (pthread)
- Math library (libm)
- For hardware mode: Serial communication libraries

## License

MIT License 