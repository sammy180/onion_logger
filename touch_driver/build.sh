# FILE: build.sh
#!/bin/bash
set -e  # Exit on error

echo "Cleaning build..."
make clean || true

echo "Building kernel module..."
make

echo "Building device tree overlay..."
make dtbo

echo "Installing module and overlay..."
sudo make install

echo "Checking driver status..."
dmesg | grep ft5426