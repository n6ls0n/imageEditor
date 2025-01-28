#!/bin/bash

# Check if build directory exists and delete it if it does
if [ -d "build" ]; then
    echo "Removing existing build directory..."
    rm -rf build
fi

# Create a new build directory
echo "Creating new build directory..."
mkdir build
cd build

# Generate build files
echo "Generating build files..."
cmake ..

# Build the project
echo "Building the project..."
cmake --build . --config Release
if [ $? -ne 0 ]; then
    echo "Build failed!"
    read -p "Press enter to continue"
    exit 1
fi

echo "Build successful! Running the executable..."

# Attempt to find and run the executable
executable=$(find . -name "imageEditor")
if [ -n "$executable" ]; then
    echo "Found executable: $executable"
    "$executable"
else
    echo "Executable not found!"
    read -p "Press enter to continue"
fi