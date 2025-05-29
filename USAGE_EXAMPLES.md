# Real-Time Sensor Data Logger - Usage Examples

## Quick Start

### 1. Build the Project
```bash
make clean
make
```

### 2. Basic Usage - Simulated Mode
```bash
# Run with default settings (60 seconds, 100ms interval)
./datalogger

# Bridge vibration monitoring for 30 seconds
echo "1" | ./datalogger --duration 30 --interval 100

# Environmental monitoring for 2 minutes
echo "2" | ./datalogger --duration 120 --interval 500
```

### 3. Hardware Mode
```bash
# Connect Arduino or sensor device to USB port
./datalogger --hardware /dev/ttyUSB0 --duration 300

# With custom settings
./datalogger --hardware /dev/ttyACM0 --duration 600 --interval 50 --threshold 2.5
```

## Application Examples

### Bridge Structural Health Monitoring

**Scenario**: Monitor a bridge for vibrations that could indicate structural issues.

```bash
# Long-term monitoring (1 hour) with high sensitivity
./datalogger --duration 3600 --interval 50 --threshold 2.0 --output bridge_main_span

# Quick inspection (5 minutes) during traffic
echo "1" | ./datalogger --duration 300 --interval 25
```

**Expected Output**:
- Real-time vibration amplitude display
- Anomaly detection for unusual vibrations
- Safety status assessment (Safe/Warning/Critical)
- Trend analysis showing increasing/decreasing patterns
- CSV log with timestamped data

### Industrial Equipment Monitoring

**Scenario**: Monitor machinery vibration and temperature for predictive maintenance.

```bash
# Monitor industrial pump for 8 hours
./datalogger --hardware /dev/ttyUSB0 --duration 28800 --interval 200 --output pump_station_1

# Quick diagnostic check
./datalogger --hardware /dev/ttyUSB0 --duration 60 --interval 50
```

### Environmental Data Collection

**Scenario**: Collect temperature, humidity, and pressure data for weather monitoring.

```bash
# 24-hour environmental monitoring
echo "2" | ./datalogger --duration 86400 --interval 1000 --output weather_station

# Greenhouse monitoring (1 hour)
echo "2" | ./datalogger --duration 3600 --interval 300
```

## Hardware Integration Examples

### Arduino Sensor Setup

1. **Upload the provided Arduino sketch** (`arduino_example.ino`)
2. **Connect sensors**:
   - LM35 temperature sensor to A0
   - Accelerometer/vibration sensor to A1
   - Power and ground connections

3. **Run the data logger**:
```bash
./datalogger --hardware /dev/ttyACM0 --duration 1800
```

### Modbus Device Integration

For Modbus-compatible sensors, the system expects data in format:
```
MB:ADDR:REG:VALUE
```

Example: `MB:01:0001:2345` (Address 1, Register 1, Value 2345)

### Custom Sensor Protocols

To add support for new sensor protocols:

1. **Modify `hardware_interface.c`**:
   - Add new parser function (e.g., `parse_custom_sensor_data`)
   - Update `parse_hardware_data` to call your parser

2. **Define your data format** and parsing logic

## Data Analysis

### CSV Output Format
```csv
Timestamp,Sensor_Type,Value,Unit,Description
2025-05-29 18:57:34.605816,Vibration,0.093410,m/s²,Bridge Vibration
2025-05-29 18:57:34.806076,Temperature,23.45,°C,Arduino Temperature
```

### Post-Processing with External Tools

```bash
# Plot data with gnuplot
gnuplot -e "set datafile separator ','; plot 'data/bridge_vibration.csv' using 3 with lines"

# Analyze with Python/pandas
python3 -c "
import pandas as pd
df = pd.read_csv('data/bridge_vibration.csv')
print(df.describe())
"

# Filter anomalies with awk
awk -F',' '$3 > 0.5 {print}' data/bridge_vibration.csv
```

## Advanced Configuration

### Custom Anomaly Detection
```bash
# Very sensitive (1.5 standard deviations)
./datalogger --threshold 1.5

# Less sensitive (5 standard deviations)
./datalogger --threshold 5.0
```

### High-Frequency Sampling
```bash
# 20Hz sampling (50ms interval)
./datalogger --interval 50 --duration 300

# 1Hz sampling (1000ms interval) for long-term monitoring
./datalogger --interval 1000 --duration 86400
```

### Multiple Simultaneous Loggers
```bash
# Terminal 1: Bridge monitoring
./datalogger --hardware /dev/ttyUSB0 --output bridge_north --duration 3600

# Terminal 2: Environmental monitoring  
./datalogger --hardware /dev/ttyUSB1 --output weather_data --duration 3600
```

## Troubleshooting

### Common Issues

1. **Permission denied on /dev/ttyUSB0**:
```bash
sudo chmod 666 /dev/ttyUSB0
# or add user to dialout group
sudo usermod -a -G dialout $USER
```

2. **No data from hardware**:
   - Check baud rate (default: 9600)
   - Verify data format matches expected protocol
   - Test with `screen /dev/ttyUSB0 9600` to see raw data

3. **High CPU usage**:
   - Increase sampling interval: `--interval 200`
   - Reduce analysis complexity in code

### Debug Mode
```bash
# Build with debug symbols
make debug

# Run with valgrind for memory checking
make memcheck
```

## Performance Considerations

### Sampling Rates
- **High frequency (10-50ms)**: Real-time control, vibration analysis
- **Medium frequency (100-500ms)**: General monitoring, anomaly detection  
- **Low frequency (1000ms+)**: Environmental monitoring, trend analysis

### File Size Management
- CSV files grow ~100 bytes per sample
- 1 hour at 100ms = 36,000 samples = ~3.6MB
- Use `--duration` to limit file sizes
- Automatic file rotation at 10MB (configurable in code)

### Memory Usage
- ~1KB per sample stored for analysis
- 1 hour at 100ms = ~36MB RAM
- Adjust buffer sizes in code if needed

## Integration with Other Systems

### Database Integration
```bash
# Import to SQLite
sqlite3 sensors.db ".mode csv" ".import data/bridge_vibration.csv vibration_data"

# Import to PostgreSQL
psql -d sensors -c "\COPY vibration_data FROM 'data/bridge_vibration.csv' CSV HEADER"
```

### Web Dashboard
Use the CSV output with web frameworks like:
- Grafana for real-time dashboards
- Node.js + Chart.js for custom interfaces
- Python Flask/Django for analysis tools

### Alert Systems
```bash
# Monitor for critical vibrations and send email
./datalogger --duration 86400 | grep "CRITICAL" | mail -s "Bridge Alert" admin@example.com
``` 