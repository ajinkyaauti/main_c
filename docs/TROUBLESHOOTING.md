# Troubleshooting Guide

Solutions for common issues you might encounter.

---

## Backend Issues

### ❌ "Port 8080 already in use"

**Problem:** Another application is already using port 8080.

**Solutions:**

**Option 1: Find and stop the process (Windows PowerShell)**
```powershell
# Find process using port 8080
netstat -ano | findstr :8080

# Kill process (replace PID with actual process ID)
taskkill /PID <PID> /F
```

**Option 2: Use a different port**

Edit `web_server.py` and change:
```python
TCP_SERVER_PORT = 8080  # Change to 8081, 8082, etc.
```

Then update the frontend to use the new port.

---

### ❌ Backend fails to build

**Problem:** CMake build fails with compilation errors.

**Solutions:**

1. **Clean and rebuild:**
```bash
cd build
cmake --clean-first .
cmake --build .
```

2. **Delete build folder and start fresh:**
```bash
rm -r build
mkdir build
cd build
cmake ..
cmake --build .
```

3. **Check if C++ compiler is installed:**
```bash
# Windows (MSVC)
cl

# Windows (MinGW)
g++ --version

# Linux/macOS
gcc --version
```

4. **Update CMake:**
```bash
cmake --version
# If < 3.10, download latest from https://cmake.org/download/
```

---

### ❌ Backend starts but crashes immediately

**Problem:** `p2p_server.exe` closes without any message.

**Solutions:**

1. **Run in PowerShell/CMD to see error message:**
```powershell
cd build
.\p2p_server.exe
```

2. **Check Windows Firewall:**
   - Go to Windows Security → Firewall & Network Protection
   - Click "Allow an app through firewall"
   - Add `p2p_server.exe` to allowed apps

3. **Run with admin privileges:**
```powershell
# Right-click PowerShell → Run as Administrator
cd C:\Users\AJINKYA\OneDrive\Desktop\project\p2p-file-transfer\build
.\p2p_server.exe
```

---

## Frontend Issues

### ❌ "ModuleNotFoundError: No module named 'flask'"

**Problem:** Flask is not installed.

**Solution:**
```bash
pip install flask flask-cors
```

Or install all dependencies:
```bash
pip install -r requirements.txt
```

**Verify installation:**
```bash
python -c "import flask; print(flask.__version__)"
```

---

### ❌ "Address already in use: ('0.0.0.0', 5000)"

**Problem:** Port 5000 is already in use.

**Solutions:**

**Option 1: Find and kill the process**
```bash
# Windows PowerShell
netstat -ano | findstr :5000
taskkill /PID <PID> /F

# Linux/macOS
lsof -i :5000
kill -9 <PID>
```

**Option 2: Use a different port**

Edit `web_server.py` at the bottom:
```python
app.run(host='0.0.0.0', port=5001, debug=True)  # Change 5000 to 5001
```

---

### ❌ Web interface loads but shows empty/broken

**Problem:** Backend is not running or not connected.

**Solutions:**

1. **Verify backend is running:**
```bash
netstat -ano | findstr :8080
```

Should show a listening process. If not, start the backend:
```bash
cd build
.\p2p_server.exe
```

2. **Check browser console for errors:**
   - Press F12 to open Developer Tools
   - Go to Console tab
   - Look for error messages (usually red)

3. **Verify connection in browser console:**
```javascript
// In browser console
fetch('http://localhost:5000/api/status')
  .then(r => r.json())
  .then(d => console.log(d))
  .catch(e => console.error(e))
```

---

### ❌ "TCP Error: No connection could be made"

**Problem:** Web server can't connect to TCP backend.

**Solutions:**

1. **Make sure backend is running:**
```bash
cd C:\Users\AJINKYA\OneDrive\Desktop\project\p2p-file-transfer\build
.\p2p_server.exe
```

2. **Verify port 8080:**
```bash
netstat -ano | findstr :8080
```

3. **Check firewall:**
   - Windows Security → Firewall & Network Protection
   - Allow `p2p_server.exe`

4. **Restart both servers:**
   - Stop backend (Ctrl+C)
   - Stop frontend (Ctrl+C)
   - Start backend first, wait 2 seconds
   - Start frontend

---

## File Upload Issues

### ❌ "Upload failed" with no clear error

**Problem:** File upload encountered an issue.

**Solutions:**

1. **Check file size limit:**
   - Default max: 100 MB
   - Edit `web_server.py`:
   ```python
   MAX_FILE_SIZE = 100 * 1024 * 1024  # Increase this
   ```

2. **Verify uploads folder exists:**
```bash
ls uploads/
# Should show peer_id folders
```

3. **Check folder permissions:**
   - Right-click `uploads` folder
   - Properties → Security → Edit
   - Ensure your user has write permissions

4. **Try a smaller file:**
   - Create a test file: `echo "test" > test.txt`
   - Try uploading that

---

### ❌ Uploaded file not appearing in list

**Problem:** File uploaded but doesn't show in "Available Files".

**Solutions:**

1. **Refresh the page:**
   - Sometimes the UI doesn't auto-update
   - Press F5 or click Refresh button

2. **Check backend logs:**
   - Look at the backend terminal
   - You should see connection messages

3. **Check files were saved:**
```bash
ls uploads/
ls uploads/<peer_id>/
```

4. **Disconnect and reconnect:**
   - Click Disconnect
   - Clear browser cache (F12 → Application → Clear All)
   - Refresh page
   - Connect again

---

## Network Issues

### ❌ "Cannot reach localhost"

**Problem:** Browser can't connect to web server.

**Solutions:**

1. **Check if frontend is running:**
```bash
# Terminal where you ran web_server.py should show:
# * Running on http://127.0.0.1:5000
```

2. **Try these URLs:**
   - `http://127.0.0.1:5000` (IP instead of localhost)
   - `http://localhost:5000`
   - Check the exact URL in terminal output

3. **Windows Firewall:**
   - Search "Allow app through firewall"
   - Find Python and allow it
   - Restart web server

4. **Try different browser:**
   - Chrome, Firefox, or Edge
   - Clear all cache
   - Try incognito/private mode

---

### ❌ Multiple browsers can't connect simultaneously

**Problem:** First browser works, but second one fails.

**Solution:**

This is likely a threading issue. The Flask app is in debug mode (single-threaded).

Edit `web_server.py` at the bottom:
```python
app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)
```

---

## Database/State Issues

### ❌ Peer data not persisting

**Problem:** When you restart servers, uploaded files are gone.

**Current Behavior:** Files are stored in memory only.

**Solution for persistent storage:**
- This is a future enhancement
- Currently data is lost on server restart
- For persistent storage, we need a database (SQLite/PostgreSQL)

---

### ❌ Old peer data still showing

**Problem:** A disconnected peer's files still appear.

**Solution:**

Restart the backend to clear all data:
```bash
# Kill backend (Ctrl+C)
# Start it again
.\p2p_server.exe
```

---

## Performance Issues

### ❌ Large file upload is slow

**Problem:** Uploading a large file takes very long.

**Solutions:**

1. **Check file size limit:**
```python
# In web_server.py, line ~19
MAX_FILE_SIZE = 100 * 1024 * 1024  # Currently 100 MB
```

2. **Disable debug mode:**
```python
# Change at bottom of web_server.py
app.run(host='0.0.0.0', port=5000, debug=False)  # Set debug=False
```

3. **Check disk space:**
```bash
# Windows PowerShell
Get-PSDrive C | Select-Object Used, Free

# Linux/macOS
df -h
```

---

### ❌ Server becomes unresponsive

**Problem:** Server stops responding to requests.

**Solution:**

1. **Restart servers:**
   - Stop backend (Ctrl+C)
   - Stop frontend (Ctrl+C)
   - Start backend first
   - Start frontend

2. **Check resource usage:**
   - Open Task Manager (Ctrl+Shift+Esc)
   - Look for high CPU/Memory usage
   - Kill the process if stuck

---

## Debugging Tips

### Enable Verbose Logging

**Backend (C++):**
Edit `src/server.cpp` to add debug output (C++ doesn't have built-in logging yet)

**Frontend (Python):**
```bash
FLASK_ENV=development python web_server.py
```

### Check Logs

**Backend logs:** Printed to terminal
```
Server started on port 8080
```

**Frontend logs:** Printed to terminal
```
* Running on http://127.0.0.1:5000
```

### Browser Console Errors

Press F12 in browser → Console tab → Look for red errors

### Test with Minimal Setup

1. Close all except these 2 terminals:
   - Backend running
   - Frontend running
2. Open browser in private/incognito mode
3. Try single operation (connect → upload → download)

---

## Still Having Issues?

1. **Check all prerequisites are installed:**
   - Python 3.8+
   - CMake 3.10+
   - C++ compiler

2. **Verify ports are free:**
```bash
netstat -ano | findstr :8080
netstat -ano | findstr :5000
```

3. **Review the log messages carefully** - they usually indicate the problem

4. **Try starting from a clean state:**
   - Delete `uploads/` folder
   - Delete `build/` folder
   - Rebuild and restart

5. **Check project files exist:**
   - `src/server.cpp`
   - `web_server.py`
   - `web/` folder with HTML files

---

## Report Issues

If you encounter a bug:
1. Note the exact error message
2. Check what you were doing (connect, upload, etc.)
3. Mention your OS and Python version
4. Include terminal output

Happy troubleshooting! 🔧
