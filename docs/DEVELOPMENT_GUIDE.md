# Development Guide

How to modify, extend, and contribute to the P2P File Transfer System.

---

## Project Structure Overview

```
p2p-file-transfer/
├── CMakeLists.txt              # Build configuration
├── web_server.py               # Python Flask app (Frontend)
├── requirements.txt            # Python dependencies
├── README.md                   # Main documentation
├── include/                    # C++ Header files
│   ├── server.h               # Server class definition
│   └── protocol.h             # Protocol constants
├── src/                        # C++ Source files
│   ├── server.cpp             # Server implementation
│   ├── protocol.cpp           # Protocol utilities
│   └── main_server.cpp        # Server entry point
├── web/                        # Web UI files
│   ├── index.html             # Main webpage
│   ├── app.js                 # Frontend JavaScript
│   └── style.css              # Styling
├── tests/                      # Test files
│   └── test_client.cpp        # TCP client test
├── uploads/                    # Uploaded files storage
└── docs/                       # Documentation (this folder)
    ├── GETTING_STARTED.md
    ├── API_DOCUMENTATION.md
    ├── TROUBLESHOOTING.md
    └── DEVELOPMENT_GUIDE.md
```

---

## Backend Development (C++)

### File: `include/server.h`

Main server class definition. Defines:
- Server class structure
- Method declarations
- Member variables

### File: `src/server.cpp`

Server implementation:
- Client connection handling
- Multi-threading logic
- Command processing
- File registry management

### Adding a New Command

**Step 1: Update Protocol**

Edit `include/protocol.h`:
```cpp
constexpr const char* CMD_SEARCH = "SEARCH";  // New command
```

**Step 2: Add Handler in Server**

Edit `src/server.cpp`, find the command processing section:
```cpp
if (command == "SEARCH") {
    std::string keyword = tokens[1];
    handleSearch(keyword);
    return;
}
```

**Step 3: Implement Handler**

Add method in `include/server.h`:
```cpp
void handleSearch(const std::string& keyword);
```

Implement in `src/server.cpp`:
```cpp
void Server::handleSearch(const std::string& keyword) {
    // Your search logic here
    std::string response = "OK [search results]";
    sendResponse(response);
}
```

**Step 4: Rebuild**

```bash
cd build
cmake --build .
```

---

## Frontend Development (Python)

### File: `web_server.py`

Main Flask application containing:
- REST API endpoints
- TCP client for backend communication
- File upload/download handling
- Session management

### Adding a New API Endpoint

**Step 1: Create the route**

```python
@app.route('/api/newfeature', methods=['GET', 'POST'])
def new_feature():
    """Description of your feature"""
    data = request.json
    
    # Process request
    result = process_data(data)
    
    # Return response
    return jsonify({
        'status': 'success',
        'data': result
    })
```

**Step 2: Update the frontend**

Edit `web/app.js` to call your new endpoint.

**Step 3: Restart the server**

```bash
python web_server.py
```

### Example: Add "Rename File" Feature

**Backend (C++):**
```cpp
// In server.cpp
} else if (command == "RENAME") {
    std::string oldName = tokens[1];
    std::string newName = tokens[2];
    // Rename file logic
    response = "OK File renamed";
}
```

**Frontend (Python):**
```python
@app.route('/api/rename', methods=['POST'])
def rename_file():
    data = request.json
    old_name = data.get('old_name')
    new_name = data.get('new_name')
    peer_id = data.get('peer_id')
    
    # Call TCP server
    response = tcp_client.send_command(f"RENAME {old_name} {new_name}")
    
    return jsonify({
        'status': 'success' if 'OK' in response else 'error',
        'message': response
    })
```

---

## Web UI Development

### File: `web/index.html`

HTML structure of the web interface.

### File: `web/app.js`

JavaScript logic:
- Button click handlers
- API calls
- DOM updates
- File drag-and-drop

### File: `web/style.css`

Styling and layout.

### Adding a New UI Button

**Step 1: Add HTML element in index.html**

```html
<button id="searchBtn">Search Files</button>
```

**Step 2: Add CSS styling in style.css**

```css
#searchBtn {
    background-color: #4CAF50;
    padding: 10px 20px;
    cursor: pointer;
}
```

**Step 3: Add JavaScript handler in app.js**

```javascript
document.getElementById('searchBtn').addEventListener('click', async function() {
    const keyword = prompt('Search for:');
    
    const response = await fetch('/api/search', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({
            keyword: keyword,
            peer_id: currentPeerId
        })
    });
    
    const data = await response.json();
    console.log('Search results:', data);
});
```

---

## Building and Testing

### Build Backend Only

```bash
cd build
cmake --build .
```

### Run Tests

```bash
# C++ tests
cd build
./tests/test_client.exe  # If exists

# Manual testing with backend running
cd .. && python web_server.py
```

### Debug Backend

**Add logging to C++:**
```cpp
#include <iostream>

std::cout << "Debug info: " << variable << std::endl;
```

Rebuild and run:
```bash
cd build
cmake --build .
.\p2p_server.exe  # Run with output visible
```

### Debug Frontend

**Browser Console:**
- Press F12
- Go to Console tab
- Errors appear in red
- Add console.log() in JavaScript

**Example:**
```javascript
// In app.js
console.log('Connected as:', peerId);
console.log('Files:', files);
```

---

## Configuration Changes

### Change Default Port

**Backend (C++):**

Edit `src/main_server.cpp` or `src/server.cpp`:
```cpp
const int PORT = 8081;  // Change from 8080
```

Rebuild:
```bash
cd build
cmake --build .
```

**Frontend (Python):**

Edit `web_server.py`:
```python
TCP_SERVER_PORT = 8081  # Match backend port
```

### Change Max File Size

Edit `web_server.py`:
```python
MAX_FILE_SIZE = 500 * 1024 * 1024  # 500 MB instead of 100 MB
```

### Change Upload Folder

Edit `web_server.py`:
```python
UPLOAD_FOLDER = 'shared_files'  # Change from 'uploads'
os.makedirs(UPLOAD_FOLDER, exist_ok=True)
```

---

## Performance Optimization

### Disable Debug Mode (Production)

Edit `web_server.py`:
```python
# Old:
app.run(host='0.0.0.0', port=5000, debug=True)

# New:
app.run(host='0.0.0.0', port=5000, debug=False, threaded=True)
```

### Use Connection Pooling

For multiple simultaneous uploads, implement connection pooling in Python:
```python
from concurrent.futures import ThreadPoolExecutor

executor = ThreadPoolExecutor(max_workers=10)
```

### C++ Optimization

Enable compiler optimizations in `CMakeLists.txt`:
```cmake
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
```

---

## Error Handling

### Backend Error Handling

```cpp
try {
    // Your code
} catch (const std::exception& e) {
    std::string response = "ERROR " + std::string(e.what());
    sendResponse(response);
}
```

### Frontend Error Handling

```python
try:
    response = tcp_client.send_command(command)
    return jsonify({'status': 'success', 'data': response})
except Exception as e:
    return jsonify({'status': 'error', 'message': str(e)})
```

### UI Error Display

```javascript
fetch(url)
    .then(r => r.json())
    .then(d => {
        if (d.status === 'error') {
            alert('Error: ' + d.message);
        } else {
            // Handle success
        }
    })
    .catch(e => console.error('Fetch failed:', e));
```

---

## Version Control (Git)

### Important Files to Track

Add to `.gitignore`:
```
build/
uploads/
venv/
__pycache__/
*.o
*.exe
.DS_Store
```

### Typical Workflow

```bash
# Make changes
git add .
git commit -m "Add new search feature"

# Before pushing, test:
cd build && cmake --build .
python web_server.py
```

---

## Documentation

### Add Code Comments

```cpp
// Before complex logic
// This function handles file validation
void validateFile(const std::string& filename) { ... }
```

```python
# Docstrings for functions
def process_upload(file, peer_id):
    """
    Process file upload from a peer.
    
    Args:
        file: Binary file object
        peer_id: ID of uploading peer
    
    Returns:
        dict: Status and file info
    """
```

### Update Documentation

When adding features:
1. Update `docs/API_DOCUMENTATION.md` with new endpoints
2. Update `README.md` with new features
3. Update `docs/DEVELOPMENT_GUIDE.md` with how to use it

---

## Common Tasks

### Add Persistent Database

Currently uses in-memory storage. To add SQLite:

```python
import sqlite3

db = sqlite3.connect('p2p_data.db')
cursor = db.cursor()

cursor.execute('''CREATE TABLE files
    (id INTEGER PRIMARY KEY,
     filename TEXT,
     owner TEXT,
     size INTEGER)''')

db.commit()
```

### Add Authentication

```python
@app.before_request
def check_auth():
    token = request.headers.get('Authorization')
    if not verify_token(token):
        return jsonify({'status': 'error', 'message': 'Unauthorized'}), 401
```

### Add Logging

```python
import logging

logging.basicConfig(filename='app.log', level=logging.INFO)
logging.info('Server started')
```

---

## Testing Checklist

Before deploying changes:

- [ ] Code compiles without errors
- [ ] All existing features still work
- [ ] New feature works as expected
- [ ] No memory leaks (C++)
- [ ] Performance is acceptable
- [ ] Error cases handled gracefully
- [ ] Documentation updated
- [ ] Code follows project style

---

## Need Help?

- Check existing code for examples
- Review git history: `git log --oneline`
- Check issue tracker or comments in code
- Test incrementally, not all at once
- Use version control to roll back if needed

Happy coding! 🚀
