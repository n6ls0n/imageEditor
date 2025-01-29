# Use latest Python base image
FROM python:latest

# Set working directory
WORKDIR /app

# Copy the custom server script
COPY custom_server.py /app

# Copy the folder containing HTML and WASM files
COPY ./build_web/web /app

# Set the port as an environment variable
ENV PORT=7060

# Expose the port for the Python HTTP server
EXPOSE $PORT

# Run the custom Python HTTP server
CMD ["python", "custom_server.py"]

# Build and run the development image
# docker build -t imageeditor:latest .
# docker run -p 7060:7060 -d imageeditor