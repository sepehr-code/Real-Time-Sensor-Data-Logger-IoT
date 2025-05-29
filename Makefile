# Real-Time Sensor Data Logger Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS = -lm -lpthread

# Directories
SRCDIR = src
INCDIR = include
OBJDIR = obj
DATADIR = data

# Target executable
TARGET = datalogger

# Source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Header files
HEADERS = $(wildcard $(INCDIR)/*.h)

# Default target
all: $(TARGET)

# Create object directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Create data directory if it doesn't exist
$(DATADIR):
	mkdir -p $(DATADIR)

# Build target executable
$(TARGET): $(OBJECTS) | $(DATADIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	$(CC) $(CFLAGS) -I$(INCDIR) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Clean everything including data files
distclean: clean
	rm -rf $(DATADIR)
	@echo "Distribution clean complete"

# Install target (copy to /usr/local/bin)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installed $(TARGET) to /usr/local/bin/"

# Uninstall target
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled $(TARGET)"

# Run with default settings (simulated mode)
run: $(TARGET)
	./$(TARGET)

# Run bridge monitoring demo
demo-bridge: $(TARGET)
	./$(TARGET) --duration 30 --interval 50

# Run environmental monitoring demo
demo-env: $(TARGET)
	./$(TARGET) --duration 60 --interval 200

# Debug build
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Release build
release: CFLAGS += -DNDEBUG -O3
release: clean $(TARGET)

# Check for memory leaks with valgrind
memcheck: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET) --duration 10

# Static analysis with cppcheck
analyze:
	cppcheck --enable=all --std=c99 $(SRCDIR)/ $(INCDIR)/

# Format code with clang-format
format:
	clang-format -i $(SRCDIR)/*.c $(INCDIR)/*.h

# Show help
help:
	@echo "Real-Time Sensor Data Logger Build System"
	@echo "=========================================="
	@echo ""
	@echo "Targets:"
	@echo "  all          - Build the project (default)"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove build artifacts and data files"
	@echo "  install      - Install to /usr/local/bin"
	@echo "  uninstall    - Remove from /usr/local/bin"
	@echo "  run          - Run with default settings"
	@echo "  demo-bridge  - Run bridge monitoring demo"
	@echo "  demo-env     - Run environmental monitoring demo"
	@echo "  debug        - Build with debug symbols"
	@echo "  release      - Build optimized release version"
	@echo "  memcheck     - Run with valgrind memory checker"
	@echo "  analyze      - Run static analysis with cppcheck"
	@echo "  format       - Format code with clang-format"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage Examples:"
	@echo "  make                                    # Build project"
	@echo "  make run                                # Run in simulated mode"
	@echo "  make demo-bridge                        # Bridge monitoring demo"
	@echo "  ./datalogger --help                     # Show program help"
	@echo "  ./datalogger --hardware /dev/ttyUSB0    # Hardware mode"

# Phony targets
.PHONY: all clean distclean install uninstall run demo-bridge demo-env debug release memcheck analyze format help

# Dependencies
$(OBJDIR)/main.o: $(INCDIR)/utils.h $(INCDIR)/sensor_simulator.h $(INCDIR)/hardware_interface.h $(INCDIR)/data_logger.h $(INCDIR)/data_analyzer.h
$(OBJDIR)/utils.o: $(INCDIR)/utils.h
$(OBJDIR)/sensor_simulator.o: $(INCDIR)/sensor_simulator.h $(INCDIR)/utils.h
$(OBJDIR)/hardware_interface.o: $(INCDIR)/hardware_interface.h $(INCDIR)/sensor_simulator.h $(INCDIR)/utils.h
$(OBJDIR)/data_logger.o: $(INCDIR)/data_logger.h $(INCDIR)/sensor_simulator.h $(INCDIR)/utils.h
$(OBJDIR)/data_analyzer.o: $(INCDIR)/data_analyzer.h $(INCDIR)/sensor_simulator.h $(INCDIR)/utils.h 