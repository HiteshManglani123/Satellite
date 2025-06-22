# Satellite Communication System

This project simulates a satellite-to-ground-station image transfer using a custom, simplified TFTP protocol over UNIX domain sockets. It is written in C and demonstrates inter-process communication, file transfer, and protocol implementation.

## Overview

The system consists of two main applications:
- **Satellite**: Acts as a TFTP server, sending an image file upon request.
- **Ground Station**: Acts as a TFTP client, requesting and receiving the image file.

The transfer protocol is implemented in a custom TFTP library. The image is transferred as a raw byte stream; no image processing is performed during transfer.

## Directory Structure

```
Satellite/
├── images/                # Contains source images (e.g., some-random-stars.bmp)
├── received-images/       # Where received images are saved (e.g., test.bmp)
├── src/
│   ├── satellite/         # Satellite application source
│   ├── ground-station/    # Ground station application source
│   ├── tftp.c, tftp.h     # TFTP protocol implementation
│   ├── image-processing.c,# Image processing code (currently unused)
│   └── image-processing.h # BMP header definitions
├── tests/                 # Test files
├── temp/                  # UNIX socket files and temp data
├── Makefile               # Build instructions
└── README.md              # Project documentation
```

## How It Works

1. **Satellite** starts and binds to a UNIX domain socket (`temp/server-socket`).
2. **Ground Station** starts and sends a read request to the satellite's socket.
3. The satellite reads an image from `images/some-random-stars.bmp` and sends it in blocks to the ground station.
4. The ground station writes the received data to `received-images/test.bmp`, sending an ACK for each block.

## Building the Project

To build the project, run:

```
make
```

This will compile both the satellite and ground station applications.

## Running the Applications

1. **Start the Satellite (server):**
   ```
   ./build/satellite
   ```
2. **Start the Ground Station (client) in another terminal:**
   ```
   ./build/ground-station
   ```

After the transfer, check `received-images/test.bmp` for the received image.

## Notes
- The image processing code is present but not currently used in the transfer process.
- Communication is local (UNIX sockets), not over a network.
- The TFTP implementation is simplified for demonstration purposes.

## Authors
- Hitesh Manglani
