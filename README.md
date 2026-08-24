# P2P File Transfer System

A peer-to-peer file transfer application with a central server for peer discovery and coordination. Built with C++ and CMake, featuring a modern web-based UI.

## Features

- ✅ Multi-threaded TCP server
- ✅ Client registration and tracking
- ✅ File registry management
- ✅ Cross-platform support (Windows/Linux/macOS)
- ✅ **Modern Web UI with drag-and-drop file upload**
- ✅ **Real-time activity logging**
- ✅ **File upload/download through web interface**
- ✅ Simple text-based protocol
- 🚧 Direct peer-to-peer connections (coming soon)

## Project Structure

```
p2p-file-transfer/
├── CMakeLists.txt          # Build configuration
├── README.md               # This file
├── PLAN.md                 # Implementation roadmap
├── web_server.py           # Flask web server (Python)
├── requirements.txt        # Python dependencies
├── start.bat               # Windows launcher script
├── start.sh                # Linux/macOS launcher script
├── include/                # Header files
│   ├── server.h           # Server class
│   └── protocol.h         # Protocol definitions
├── src/                    # Source files
│   ├── server.cpp         # Server implementation
│   ├── protocol.cpp       # Protocol utilities
│   └── main_server.cpp    # Server entry point
### For C++ Server
- **CMake** 3.10 or higher
- **C++17** compatible compiler:
  - GCC 7+ (Linux/macOS)
  - Clang 5+ (macOS)
  - MSVC 2017+ (Windows)
  - MinGW-w64 (Windows)

### For Web Interface
- **Python** 3.8 or higher
- **pip** (Python package manager

## Requirements

- **CMake** 3.10 or higher
- **C++17** compatible compiler:
  - GCC 7+ (Linux/macOS)
  - Clang 5+ (macOS)
  - MSVC 2017+ (Windows)
  - MinGW-w64 (Windows)

## Building

### Windows (Visual Studio)

```bash
# Create build directory
mkdir build
cd build

# Generate Visual Studio project
cmake ..

# Build
cmake --build . --config Release

# Run
.\Release\p2p_server.exe
```

### Windows (MinGW)

```bash
mkdir build
cd build
cmake -G "MinGW Makefiles" ..
cmake --build .
.\p2p_server.exe
```

### Linux/macOS

```bash
mkdir build
cd build
cmake ..
make
./p2p_server
```

## Usage

### 🚀 Quick Start with Web Interface (Recommended)

The easiest way to use the system is with the web interface:

#### Windows
```bash
# Double-click start.bat or run:
start.bat
```

#### Linux/macOS
```bash
chmod +x start.sh
./start.sh
```

This will:
1. Start the C++ TCP server on port 8080
2. Start the Python web server on port 5000
3. Open http://localhost:5000 in your browser

### Using the Web Interface

1. **Open your browser** to `http://localhost:5000`
2. **Enter a Peer ID** (e.g., "Alice", "Bob")
3. **Click Connect**
4. **Upload files** by dragging & dropping or clicking the upload area
5. **View available files** from other peers
6. **Download files** with one click

![Web UI Features](web/screenshot.png)

### Manual Setup

If you want to run components separately:

#### Step 1: Build and Run TCP Server

```bash
# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run (Windows)
./p2p_server.exe

# Run (Linux/macOS)
./p2p_server
```

#### Step 2: Install Python Dependencies

```bash
pip install -r requirements.txt
```

#### Step 3: Run Web Server

```bash
python web_server.py
# Or: python3 web_server.py (Linux/macOS)
```

#### Step 4: Access Web Interface

Open your browser to: **http://localhost:5000**

### Command-Line Testing

### Command-Line Testing

You can also test the TCP server directly using telnet or the test client:

```bash
# Connect to server
telnet localhost 8080

# Try these commands:
CONNECT peer1
LIST
UPLOAD myfile.txt 1024
LIST
DOWNLOAD myfile.txt
DISCONNECT
```

### Protocol Commands

| Command | Format | Description |
|---------|--------|-------------|
| `CONNECT` | `CONNECT <peer_id>` | Register with the server |
| `LIST` | `LIST` | Get list of all available files |
| `UPLOAD` | `UPLOAD <filename> <size>` | Register a file you're sharing |
| `DOWNLOAD` | `DOWNLOAD <filename>` | Find who has a specific file |
| `DISCONNECT` | `DISCONNECT` | Disconnect from server |

### Response Format

All responses follow this format:
```
<STATUS> [data]
```

- `OK` - Success with optional data
- `ERROR` - Failure with error message
- `READY` - Server ready for data

## Example Session

```
# Client 1 connects and uploads a file
Client 1: CONNECT alice
Server:   OK Connected

Client 1: UPLOAD document.pdf 2048576
Server:   OK Upload registered

# Client 2 connects and searches for the file
Client 2: CONNECT bob
Server:   OK Connected

Client 2: LIST
Server:   OK 1 alice:document.pdf

Client 2: DOWNLOAD document.pdf
Server:   OK File found: alice
```

## Development Roadmap

See [PLAN.md](PLAN.md) for the detailed implementation plan.

### Phase 1: Basic Server ✅ (Current)
- TCP server with multi-threading
- Client registration
- File registry
- Basic command protocol

### Phase 2: File Transfer 🚧 (Next)
- Binary file upload/download
- Chunked transfers
- Progress tracking
- MD5 checksums

### Phase 3: P2P Direct Transfer
- Peer discovery
- Direct peer connections
- NAT traversal

### Phase 4: Advanced Features
- File search
- Resume support
- Encryption
- Compression

## Troubleshooting

### Windows: "Address already in use"
The port is already occupied. Try a different port or wait a few seconds for the previous server to fully close.

### Linux/macOS: Permission denied on port < 1024
Ports below 1024 require root privileges. Use a port >= 1024 or run with sudo (not recommended).

### Connection refused
Make sure the server is running and check your firewall settings.

## Contributing

This is a learning project! Feel free to:
- Add features from the roadmap
- Improve error handling
- Add unit tests
- Enhance the protocol
- Add a GUI client

## License

MIT License - Feel free to use for learning and experimentation.

## Next Steps

1. **Build a Simple Client**: Create a C++ client that connects to the server
2. **Add File Transfer**: Implement actual binary file sending/receiving
3. **Add P2P Mode**: Allow clients to connect directly to each other
4. **Create a GUI**: Build a graphical interface using Qt or similar

---

**Happy Coding!** 🚀
