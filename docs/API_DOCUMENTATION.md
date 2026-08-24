# API Documentation

Complete reference for all REST APIs and TCP commands in the P2P File Transfer System.

---

## REST API (Python Web Server - Port 5000)

All requests use JSON for request/response bodies.

### 1. Connect Peer

**Endpoint:** `POST /api/connect`

**Description:** Register a peer with the system

**Request:**
```json
{
  "peer_id": "Alice"
}
```

**Response (Success):**
```json
{
  "status": "success",
  "message": "Connected successfully",
  "peer_id": "Alice"
}
```

**Response (Error):**
```json
{
  "status": "error",
  "message": "Peer ID required"
}
```

**Example (cURL):**
```bash
curl -X POST http://localhost:5000/api/connect \
  -H "Content-Type: application/json" \
  -d '{"peer_id": "Alice"}'
```

---

### 2. Disconnect Peer

**Endpoint:** `POST /api/disconnect`

**Description:** Unregister a peer from the system

**Request:**
```json
{
  "peer_id": "Alice"
}
```

**Response:**
```json
{
  "status": "success"
}
```

**Example (cURL):**
```bash
curl -X POST http://localhost:5000/api/disconnect \
  -H "Content-Type: application/json" \
  -d '{"peer_id": "Alice"}'
```

---

### 3. Upload File

**Endpoint:** `POST /api/upload`

**Description:** Upload a file to share

**Parameters (Form Data):**
- `file` - Binary file content
- `peer_id` - Peer ID uploading the file

**Response (Success):**
```json
{
  "status": "success",
  "message": "File uploaded successfully",
  "filename": "document.pdf",
  "size": 2048576
}
```

**Response (Error):**
```json
{
  "status": "error",
  "message": "Peer ID required"
}
```

**Example (cURL):**
```bash
curl -X POST http://localhost:5000/api/upload \
  -F "file=@/path/to/file.pdf" \
  -F "peer_id=Alice"
```

**Example (Python):**
```python
import requests

with open('file.pdf', 'rb') as f:
    files = {'file': f}
    data = {'peer_id': 'Alice'}
    response = requests.post('http://localhost:5000/api/upload', 
                            files=files, data=data)
    print(response.json())
```

---

### 4. List Files

**Endpoint:** `GET /api/list`

**Description:** Get all available files from all peers

**Response:**
```json
{
  "status": "success",
  "files": [
    {
      "filename": "document.pdf",
      "size": 2048576,
      "owner": "Alice"
    },
    {
      "filename": "image.jpg",
      "size": 1024000,
      "owner": "Bob"
    }
  ]
}
```

**Example (cURL):**
```bash
curl http://localhost:5000/api/list
```

**Example (Python):**
```python
import requests

response = requests.get('http://localhost:5000/api/list')
files = response.json()['files']
for file in files:
    print(f"{file['filename']} ({file['size']} bytes) - Owner: {file['owner']}")
```

---

### 5. Download File

**Endpoint:** `GET /api/download`

**Description:** Download a file from a peer

**Query Parameters:**
- `filename` - Name of the file to download
- `owner` - Peer ID who owns the file

**Response:** Binary file content

**HTTP Status:**
- `200` - File found and transferred
- `404` - File not found

**Example (cURL):**
```bash
curl "http://localhost:5000/api/download?filename=document.pdf&owner=Alice" -O
```

**Example (Python):**
```python
import requests

response = requests.get('http://localhost:5000/api/download',
                       params={'filename': 'document.pdf', 'owner': 'Alice'})

if response.status_code == 200:
    with open('downloaded_file.pdf', 'wb') as f:
        f.write(response.content)
    print("File downloaded successfully!")
else:
    print("File not found")
```

---

### 6. Server Status

**Endpoint:** `GET /api/status`

**Description:** Get server statistics

**Response:**
```json
{
  "status": "success",
  "total_files": 5,
  "total_peers": 3,
  "connected_peers": 2
}
```

**Metrics:**
- `total_files` - Total files shared across all peers
- `total_peers` - Total unique peer IDs registered
- `connected_peers` - Currently connected peers

**Example (cURL):**
```bash
curl http://localhost:5000/api/status
```

---

## TCP Protocol (C++ Server - Port 8080)

Raw TCP text commands. Each command must end with a newline (`\n`).

### Connection Commands

#### CONNECT

**Format:** `CONNECT <peer_id>`

**Response:** `OK Connected as <peer_id>`

**Example:**
```
> CONNECT Alice
< OK Connected as Alice
```

---

#### DISCONNECT

**Format:** `DISCONNECT`

**Response:** `OK Disconnected`

**Example:**
```
> DISCONNECT
< OK Disconnected
```

---

### Query Commands

#### LIST

**Format:** `LIST`

**Response:** `OK [JSON array of files]`

**Example:**
```
> LIST
< OK [{"filename":"file1.txt","size":1024,"owner":"Alice"}]
```

---

#### DOWNLOAD

**Format:** `DOWNLOAD <filename>`

**Response:** `OK <owner> <size>` or `ERROR File not found`

**Example:**
```
> DOWNLOAD document.pdf
< OK Alice 2048576

> DOWNLOAD notexist.txt
< ERROR File not found
```

---

### File Management

#### UPLOAD

**Format:** `UPLOAD <filename> <size>`

**Response:** `OK File registered` or `ERROR Invalid format`

**Example:**
```
> UPLOAD document.pdf 2048576
< OK File registered: document.pdf (2048576 bytes)
```

---

## HTTP Status Codes

| Code | Meaning |
|------|---------|
| `200` | Success |
| `400` | Bad request (missing parameters) |
| `404` | Resource not found |
| `500` | Server error |

---

## Error Responses

All error responses follow this format:

```json
{
  "status": "error",
  "message": "Description of what went wrong"
}
```

**Common Errors:**
- `"Peer ID required"` - Missing peer_id in request
- `"No file provided"` - No file in upload request
- `"File not found"` - Requested file doesn't exist
- `"Upload failed: [reason]"` - File upload encountered an issue

---

## Response Formats

### Success Response
```json
{
  "status": "success",
  "message": "Operation completed",
  "data": {...}
}
```

### Error Response
```json
{
  "status": "error",
  "message": "Description of error"
}
```

---

## Rate Limits

Currently **no rate limiting** is implemented. In production, consider:
- Limiting requests per peer per minute
- Limiting file upload size
- Implementing authentication/authorization

---

## WebSocket Support

Not yet implemented. Currently uses HTTP polling for updates.

Future enhancement: Real-time notifications via WebSockets.

---

## Examples

### Complete Workflow (cURL)

```bash
# 1. Connect as Alice
curl -X POST http://localhost:5000/api/connect \
  -H "Content-Type: application/json" \
  -d '{"peer_id": "Alice"}'

# 2. Upload a file
curl -X POST http://localhost:5000/api/upload \
  -F "file=@myfile.txt" \
  -F "peer_id=Alice"

# 3. List all files
curl http://localhost:5000/api/list

# 4. Connect as Bob and download Alice's file
curl -X POST http://localhost:5000/api/connect \
  -H "Content-Type: application/json" \
  -d '{"peer_id": "Bob"}'

curl "http://localhost:5000/api/download?filename=myfile.txt&owner=Alice" -O

# 5. Check status
curl http://localhost:5000/api/status
```

---

## Need Help?

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common issues.
