# Getting Started with P2P File Transfer System

A beginner-friendly guide to get the system up and running in minutes.

## Prerequisites

Make sure you have installed:
- **Python 3.8+** - [Download](https://www.python.org/downloads/)
- **CMake 3.10+** - [Download](https://cmake.org/download/)
- **C++ Compiler** - One of:
  - Visual Studio 2017+ (includes MSVC)
  - MinGW-w64 (for Windows)
  - GCC 7+ (for Linux/macOS)

Verify installations:
```bash
python --version
cmake --version
```

---

## Step 1: Build the C++ Backend (First Time Only)

Navigate to the project directory:
```bash
cd C:\Users\AJINKYA\OneDrive\Desktop\project\p2p-file-transfer
```

Create and build:
```bash
mkdir build
cd build
cmake ..
cmake --build .
```

You should see `p2p_server.exe` created in the `build` folder.

---

## Step 2: Install Python Dependencies

Go back to the project root:
```bash
cd C:\Users\AJINKYA\OneDrive\Desktop\project\p2p-file-transfer
```

Install required packages:
```bash
pip install flask flask-cors
```

Or install from requirements:
```bash
pip install -r requirements.txt
```

---

## Step 3: Start the Backend Server

Open a **PowerShell or CMD terminal** and run:
```bash
cd C:\Users\AJINKYA\OneDrive\Desktop\project\p2p-file-transfer\build
.\p2p_server.exe
```

You should see:
```
=== P2P File Transfer Server ===
Starting server on port 8080...
Server started on port 8080
Server is running. Press Ctrl+C to stop.
```

✅ **Backend is running on port 8080**

---

## Step 4: Start the Frontend (Web Server)

Open a **new terminal** (keep the backend running) and run:
```bash
cd C:\Users\AJINKYA\OneDrive\Desktop\project\p2p-file-transfer
python web_server.py
```

You should see:
```
* Running on http://127.0.0.1:5000
```

✅ **Frontend is running on port 5000**

---

## Step 5: Access the Web Interface

Open your browser and go to:
```
http://localhost:5000
```

You should see the P2P File Transfer UI with:
- Peer ID input field
- Connect button
- File upload area
- Available files list

---

## Step 6: Test the System

### Connect as a Peer
1. Enter a Peer ID (e.g., "Alice")
2. Click **Connect**
3. You should see "Connected successfully"

### Upload a File
1. Click the **upload area** or drag & drop a file
2. Select any file from your computer
3. Wait for upload to complete

### View Files
1. Click **Refresh** or check the "Available Files" section
2. You should see your uploaded file listed

### Test with Multiple Peers
1. Open another browser window/tab (incognito mode recommended)
2. Go to http://localhost:5000
3. Enter a different Peer ID (e.g., "Bob")
4. Connect Bob
5. You should see Alice's files available to Bob
6. Download Alice's file as Bob

---

## Quick Troubleshooting

### ❌ Backend won't start
- Make sure port 8080 is not in use
- Check firewall settings
- Ensure you're in the `build` folder

### ❌ Flask errors (ModuleNotFoundError)
- Install Flask: `pip install flask flask-cors`
- Use `pip list` to verify installation

### ❌ Can't connect to localhost:5000
- Make sure backend is running first
- Check that port 5000 is free
- Try refreshing the page

### ❌ File upload fails
- Check that uploads folder has write permissions
- Ensure file size < 100MB (default limit)
- Backend must be running

For more help, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md)

---

## What's Running

| Component | Port | Purpose |
|-----------|------|---------|
| **C++ TCP Server** | 8080 | Handles peer connections & file registry |
| **Python Web Server** | 5000 | Provides REST API & web interface |
| **Web Browser** | — | User interface at http://localhost:5000 |

---

## Next Steps

- Read [API_DOCUMENTATION.md](API_DOCUMENTATION.md) to understand the REST APIs
- Check [DEVELOPMENT_GUIDE.md](DEVELOPMENT_GUIDE.md) to modify the code
- See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common issues

Happy coding! 🚀
