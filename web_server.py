"""
P2P File Transfer Web Server
A Flask-based web interface for the P2P file transfer system
"""

from flask import Flask, request, jsonify, send_file, send_from_directory
from flask_cors import CORS
import socket
import os
import json
from pathlib import Path
from werkzeug.utils import secure_filename

app = Flask(__name__)
CORS(app)

# Configuration
TCP_SERVER_HOST = 'localhost'
TCP_SERVER_PORT = 8080
UPLOAD_FOLDER = 'uploads'
WEB_FOLDER = 'web'
MAX_FILE_SIZE = 100 * 1024 * 1024  # 100 MB

# Ensure upload directory exists
os.makedirs(UPLOAD_FOLDER, exist_ok=True)

# File registry (peer_id -> [files])
file_registry = {}
peer_connections = {}


class TCPClient:
    """Simple TCP client to communicate with the P2P server"""
    
    def __init__(self, host, port):
        self.host = host
        self.port = port
    
    def send_command(self, command):
        """Send a command to the TCP server and get response"""
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((self.host, self.port))
                s.sendall((command + '\n').encode())
                response = s.recv(4096).decode().strip()
                return response
        except Exception as e:
            print(f"TCP Error: {e}")
            return f"ERROR {str(e)}"


tcp_client = TCPClient(TCP_SERVER_HOST, TCP_SERVER_PORT)


@app.route('/')
def index():
    """Serve the main web interface"""
    return send_from_directory(WEB_FOLDER, 'index.html')


@app.route('/<path:path>')
def serve_static(path):
    """Serve static files (CSS, JS)"""
    return send_from_directory(WEB_FOLDER, path)


@app.route('/api/connect', methods=['POST'])
def connect():
    """Connect a peer to the server"""
    data = request.json
    peer_id = data.get('peer_id')
    
    if not peer_id:
        return jsonify({'status': 'error', 'message': 'Peer ID required'})
    
    # Send CONNECT command to TCP server
    response = tcp_client.send_command(f"CONNECT {peer_id}")
    
    if response.startswith('OK'):
        peer_connections[peer_id] = True
        if peer_id not in file_registry:
            file_registry[peer_id] = []
        
        return jsonify({
            'status': 'success',
            'message': 'Connected successfully',
            'peer_id': peer_id
        })
    else:
        return jsonify({
            'status': 'error',
            'message': response
        })


@app.route('/api/disconnect', methods=['POST'])
def disconnect():
    """Disconnect a peer from the server"""
    data = request.json
    peer_id = data.get('peer_id')
    
    if peer_id in peer_connections:
        del peer_connections[peer_id]
    
    tcp_client.send_command("DISCONNECT")
    
    return jsonify({'status': 'success'})


@app.route('/api/upload', methods=['POST'])
def upload_file():
    """Handle file upload"""
    if 'file' not in request.files:
        return jsonify({'status': 'error', 'message': 'No file provided'})
    
    file = request.files['file']
    peer_id = request.form.get('peer_id')
    
    if not peer_id:
        return jsonify({'status': 'error', 'message': 'Peer ID required'})
    
    if file.filename == '':
        return jsonify({'status': 'error', 'message': 'No file selected'})
    
    try:
        # Secure the filename
        filename = secure_filename(file.filename)
        
        # Create peer directory
        peer_dir = os.path.join(UPLOAD_FOLDER, peer_id)
        os.makedirs(peer_dir, exist_ok=True)
        
        # Save file
        filepath = os.path.join(peer_dir, filename)
        file.save(filepath)
        
        # Get file size
        filesize = os.path.getsize(filepath)
        
        # Register with TCP server
        response = tcp_client.send_command(f"UPLOAD {filename} {filesize}")
        
        # Update local registry
        if peer_id not in file_registry:
            file_registry[peer_id] = []
        
        file_registry[peer_id].append({
            'filename': filename,
            'size': filesize,
            'path': filepath
        })
        
        return jsonify({
            'status': 'success',
            'message': 'File uploaded successfully',
            'filename': filename,
            'size': filesize
        })
    
    except Exception as e:
        return jsonify({
            'status': 'error',
            'message': f'Upload failed: {str(e)}'
        })


@app.route('/api/list', methods=['GET'])
def list_files():
    """List all available files"""
    files = []
    
    for peer_id, peer_files in file_registry.items():
        for file_info in peer_files:
            files.append({
                'filename': file_info['filename'],
                'size': file_info['size'],
                'owner': peer_id
            })
    
    return jsonify({
        'status': 'success',
        'files': files
    })


@app.route('/api/download', methods=['GET'])
def download_file():
    """Download a file"""
    filename = request.args.get('filename')
    owner = request.args.get('owner')
    
    if not filename or not owner:
        return jsonify({'status': 'error', 'message': 'Filename and owner required'}), 400
    
    # Find the file
    if owner in file_registry:
        for file_info in file_registry[owner]:
            if file_info['filename'] == filename:
                filepath = file_info['path']
                if os.path.exists(filepath):
                    return send_file(filepath, as_attachment=True, download_name=filename)
    
    return jsonify({'status': 'error', 'message': 'File not found'}), 404


@app.route('/api/status', methods=['GET'])
def status():
    """Get server status"""
    total_files = sum(len(files) for files in file_registry.values())
    total_peers = len(file_registry)
    
    return jsonify({
        'status': 'success',
        'total_files': total_files,
        'total_peers': total_peers,
        'connected_peers': len(peer_connections)
    })


if __name__ == '__main__':
    print("=" * 50)
    print("P2P File Transfer Web Server")
    print("=" * 50)
    print(f"Web Interface: http://localhost:5000")
    print(f"TCP Server: {TCP_SERVER_HOST}:{TCP_SERVER_PORT}")
    print(f"Upload Folder: {os.path.abspath(UPLOAD_FOLDER)}")
    print("=" * 50)
    print("\nMake sure the TCP server is running on port 8080!")
    print("Starting web server...\n")
    
    app.run(host='0.0.0.0', port=5000, debug=True)
