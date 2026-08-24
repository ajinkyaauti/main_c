#!/bin/bash

echo "===================================="
echo "P2P File Transfer System - Launcher"
echo "===================================="
echo ""

# Check if Python is installed
if ! command -v python3 &> /dev/null && ! command -v python &> /dev/null; then
    echo "Error: Python 3 is not installed"
    echo "Please install Python 3.8 or higher"
    exit 1
fi

# Use python3 if available, otherwise python
PYTHON_CMD=$(command -v python3 || command -v python)

# Check if virtual environment exists
if [ ! -d "venv" ]; then
    echo "Creating virtual environment..."
    $PYTHON_CMD -m venv venv
    echo "Installing dependencies..."
    source venv/bin/activate
    pip install -r requirements.txt
else
    echo "Activating virtual environment..."
    source venv/bin/activate
fi

echo ""
echo "Starting P2P File Transfer System..."
echo ""

# Check if TCP server is built
if [ ! -f "build/p2p_server" ] && [ ! -f "build/p2p_server.exe" ]; then
    echo "TCP Server not found. Building..."
    mkdir -p build
    cd build
    cmake ..
    cmake --build .
    cd ..
fi

echo "Step 1: Starting TCP Server (C++)..."
cd build
if [ -f "p2p_server.exe" ]; then
    ./p2p_server.exe &
else
    ./p2p_server &
fi
TCP_PID=$!
cd ..
sleep 2

echo "Step 2: Starting Web Server (Python)..."
echo ""
echo "Web Interface will be available at: http://localhost:5000"
echo ""
echo "Press Ctrl+C to stop both servers"
echo ""

# Trap Ctrl+C to kill both processes
trap "kill $TCP_PID; exit" INT

python web_server.py

# Kill TCP server when web server stops
kill $TCP_PID
