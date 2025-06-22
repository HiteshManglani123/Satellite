CC = cc
CFLAGS = -Wall -Wextra -g
LDFLAGS =

SRC_DIR = src
BUILD_DIR = build
TEST_DIR = tests

# Source files
TFTP_SRC = $(SRC_DIR)/tftp.c
SATELLITE_SRC = $(SRC_DIR)/satellite/satellite.c
GROUND_STATION_SRC = $(SRC_DIR)/ground-station/ground-station.c
IMAGE_PROCESSING_SRC = $(SRC_DIR)/image-processing.c

# Test files
TFTP_TEST = $(TEST_DIR)/tftp_test.c
IMAGE_PROCESSING_TEST = $(TEST_DIR)/image_processing_test.c
GROUND_STATION_TEST = $(TEST_DIR)/ground_station_test.c
SATELLITE_TEST = $(TEST_DIR)/satellite_test.c

# Objects
TFTP_OBJ = $(BUILD_DIR)/tftp.o
SATELLITE_OBJ = $(BUILD_DIR)/satellite.o
GROUND_STATION_OBJ = $(BUILD_DIR)/ground-station.o
IMAGE_PROCESSING_OBJ = $(BUILD_DIR)/image-processing.o

# Executables
SATELLITE = $(BUILD_DIR)/satellite
GROUND_STATION = $(BUILD_DIR)/ground-station
TFTP_TEST_EXE = $(BUILD_DIR)/tftp_test
IMAGE_PROCESSING_TEST_EXE = $(BUILD_DIR)/image_processing_test
GROUND_STATION_TEST_EXE = $(BUILD_DIR)/ground_station_test
SATELLITE_TEST_EXE = $(BUILD_DIR)/satellite_test

# Create build directory if it doesn't exist
$(shell mkdir -p $(BUILD_DIR))

# Default target
all: $(SATELLITE) $(GROUND_STATION)

# Build satellite
$(SATELLITE): $(SATELLITE_SRC) $(TFTP_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Build ground station
$(GROUND_STATION): $(GROUND_STATION_SRC) $(TFTP_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Build TFTP object
$(TFTP_OBJ): $(TFTP_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Build image processing object
$(IMAGE_PROCESSING_OBJ): $(IMAGE_PROCESSING_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Build tests
test: $(TFTP_TEST_EXE) $(IMAGE_PROCESSING_TEST_EXE) $(GROUND_STATION_TEST_EXE) $(SATELLITE_TEST_EXE)
	./$(TFTP_TEST_EXE)
	./$(IMAGE_PROCESSING_TEST_EXE)
	./$(GROUND_STATION_TEST_EXE)
	./$(SATELLITE_TEST_EXE)

# Build test executables
$(TFTP_TEST_EXE): $(TFTP_TEST) $(TFTP_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(IMAGE_PROCESSING_TEST_EXE): $(IMAGE_PROCESSING_TEST) $(IMAGE_PROCESSING_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(GROUND_STATION_TEST_EXE): $(GROUND_STATION_TEST) $(TFTP_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

$(SATELLITE_TEST_EXE): $(SATELLITE_TEST) $(TFTP_OBJ)
	$(CC) $(CFLAGS) $^ -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)/* *.gcda *.gcno *.gcov coverage.info coverage-html

.PHONY: all test clean
