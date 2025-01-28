#!/bin/bash

# Function to activate Emscripten
activate_emscripten() {
    # Update this path to where your Emscripten installation is located
    EMSDK_PATH="$HOME/mulVid/libs/emscripten"  # Adjust this path as needed
    
    if [ -d "$EMSDK_PATH" ]; then
        echo "Activating latest Emscripten SDK..."
        cd "$EMSDK_PATH"
        ./emsdk activate latest
        source ./emsdk_env.sh
        cd - > /dev/null  # Return to original directory
    else
        echo "Error: Emscripten not found at $EMSDK_PATH"
        echo "Please install Emscripten or update the EMSDK_PATH in the script."
        exit 1
    fi
}

# Check if Emscripten is activated
if ! command -v emcc &> /dev/null; then
    echo "Emscripten is not activated. Attempting to activate..."
    activate_emscripten
fi

# Verify Emscripten is now available
if ! command -v emcc &> /dev/null; then
    echo "Error: Failed to activate Emscripten. Please check your installation."
    exit 1
fi

echo "Emscripten is active. Proceeding with the build..."

# Check if build_web directory exists and delete it if it does
if [ -d "build_web" ]; then
    echo "Removing existing build_web directory..."
    rm -rf build_web
fi

# Create a new build_web directory
echo "Creating new build_web directory..."
mkdir build_web
cd build_web

# Generate build files using Emscripten
echo "Generating build files with Emscripten..."
emcmake cmake -DCMAKE_BUILD_TYPE=Release ..
if [ $? -ne 0 ]; then
    echo "CMake configuration failed!"
    read -p "Press enter to continue"
    exit 1
fi

# Build the project
echo "Building the project..."
emmake cmake --build . -j$(nproc)
if [ $? -ne 0 ]; then
    echo "Build failed!"
    read -p "Press enter to continue"
    exit 1
fi

echo "Build successful!"
echo "You can now deploy the generated HTML and JavaScript files."

# Optional: Add a command to serve the built project
# For example, using Python's built-in HTTP server:
# echo "Starting a local server..."
# python3 -m http.server 8000