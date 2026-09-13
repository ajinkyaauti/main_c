# P2P File Upload System - Implementation Plan

## Overview
A peer-to-peer file transfer system where clients can upload and download files through a central server that facilitates connections.

## Architecture

### Phase 1: Basic Server (Current Phase)
- **TCP Server**: Listens for client connections
- **Connection Management**: Handle multiple clients concurrently
- **Basic Protocol**: Simple text-based commands
- **File Registry**: Track available files and their locations

### Phase 2: File Upload/Download
- **File Transfer Protocol**: Binary file transfer
- **Chunking**: Split large files into chunks
- **Progress Tracking**: Monitor upload/download progress
- **Error Handling**: Retry failed transfers

### Phase 3: P2P Direct Transfer
- **Peer Discovery**: Clients can find each other
- **Direct Connection**: Peers connect directly for file transfer
- **NAT Traversal**: Handle firewall/NAT issues (hole punching)

### Phase 4: Advanced Features
- **File Search**: Search for files across network
- **Resume Support**: Resume interrupted transfers
- **Encryption**: Secure file transfers
- **Compression**: Compress files before transfer

## Current Implementation: Basic Server

### Features
1. **Multi-threaded Server**
   - Accept multiple client connections
   - Thread pool or async I/O for scalability

2. **Command Protocol**
   - `CONNECT <peer_id>`: Client registration
   - `LIST`: List available files
   - `UPLOAD <filename> <size>`: Initiate file upload
   - `DOWNLOAD <filename>`: Request file download
   - `DISCONNECT`: Clean disconnect

3. **Data Structures**
   - Client registry (ID, IP, port, file list)
   - File metadata (name, size, owner, chunks)

### Technology Stack
- **Language**: C++17
- **Build System**: CMake
- **Networking**: Platform sockets (Winsock on Windows, BSD sockets on Linux)
- **Threading**: C++11 std::thread
- **Data Format**: JSON for metadata, binary for files

## Directory Structure
```
p2p-file-transfer/
├── CMakeLists.txt
├── README.md
├── PLAN.md
├── include/
│   ├── server.h
│   ├── client.h
│   ├── protocol.h
│   └── file_manager.h
├── src/
│   ├── server.cpp
│   ├── client.cpp
│   ├── protocol.cpp
│   ├── file_manager.cpp
│   └── main_server.cpp
└── tests/
    └── test_client.cpp
```

## Next Steps
1. ✅ Create basic TCP server
2. Implement client connection handling
3. Add file upload functionality
4. Add file download functionality
5. Implement P2P discovery
6. Add direct peer connections

## Remaining Direct Transfer Plan

This section defines the implementation plan for the remaining work:

- Implement a direct peer file server.
- Add encrypted peer transfers.
- Add resumable downloads and hash verification.

The tracker remains responsible for authentication, peer discovery, file metadata, visibility, and transfer authorization. The tracker must not carry file contents during a normal transfer.

### Target transfer architecture

```text
Requester Peer                  Tracker                  Owner Peer
         |                           |                         |
         |-- SEARCH/DOWNLOAD ------->|                         |
         |<-- peer address + token --|                         |
         |                           |                         |
         |------------- TLS connection ----------------------->|
         |<---------- authorized file ranges -----------------|
         |                           |                         |
         |-- HASH/verification locally                         |
```

The tracker response for a downloadable file should contain metadata similar to:

```json
{
   "file_id": "stable-file-id",
   "filename": "movie.mp4",
   "size_bytes": 104857600,
   "content_hash": "sha256:...",
   "peer_id": "owner-peer",
   "address": "192.168.1.20",
   "port": 9000,
   "transfer_token": "short-lived-token",
   "expires_at": "2026-09-11T12:00:00Z"
}
```

## Phase 1: Peer File Server

### 1.1 Peer server responsibilities

Each peer will expose a separate transfer listener. It must:

- Listen on a configurable address and port.
- Accept multiple transfer connections with bounded concurrency.
- Serve only files registered by that peer.
- Resolve every requested file against the configured storage root.
- Reject absolute paths, `..` components, symlinks escaping storage, and unknown file IDs.
- Require an authorization token before sending file bytes.
- Enforce maximum concurrent transfers and bandwidth limits.
- Close idle or malformed connections after a timeout.

The tracker connection on port `8080` and the peer transfer listener should remain separate. Tracker commands exchange metadata; peer connections exchange binary file data.

### 1.2 Transfer protocol

Use a framed request header followed by binary data. Do not use the current whitespace command parser for file bytes.

Example request:

```json
{
   "version": 1,
   "request_id": "request-id",
   "operation": "GET",
   "file_id": "stable-file-id",
   "offset": 0,
   "length": 1048576,
   "transfer_token": "short-lived-token"
}
```

Recommended response sequence:

1. Length-prefixed response header.
2. Response status and metadata.
3. Length-prefixed binary chunks.
4. Transfer completion frame containing bytes sent and the expected hash.

Required limits:

- Maximum header size.
- Maximum requested range length.
- Maximum total transfer size.
- Maximum chunk size.
- Maximum request time and idle time.

### 1.3 File identity

Filenames must not be the primary identity of a file. Add:

- `file_id`: stable random identifier.
- `filename`: display name.
- `size_bytes`: expected size.
- `content_hash`: complete-content SHA-256 hash.
- `owner_user_id` and `owner_peer_id`.
- `visibility`: `public` or `private`.

Two files with the same filename must be allowed when their file IDs or owners differ.

### 1.4 Implementation tasks

- Add `PeerFileServer` and `PeerFileServerConfig` classes.
- Add a peer transfer listener separate from `Server` tracker sockets.
- Add a `FileManager` abstraction for safe open, stat, range read, and hash operations.
- Add a bounded worker strategy for transfer connections.
- Add a transfer connection timeout.
- Add a maximum transfer size and concurrent-transfer limit.
- Add peer registration fields for transfer address, port, and protocol version.
- Add a health/heartbeat update so the tracker knows when a peer is available.

### Phase 1 acceptance criteria

- A peer can serve an authorized public file to another peer.
- An unknown file ID is rejected without reading arbitrary paths.
- A path traversal attempt is rejected.
- An unauthorized or expired token is rejected.
- Slow clients do not block the listener indefinitely.
- The peer server shuts down without leaving worker threads behind.

## Phase 2: Transfer Authorization

### 2.1 Token issuance

The tracker issues a short-lived token only after checking:

- The requester is authenticated.
- The requested file exists.
- The file is public or the requester has permission.
- The owner peer is online.
- The requested operation is allowed.

The token should be bound to:

- `file_id`.
- Requester user and peer IDs.
- Owner peer ID.
- Operation, such as `GET`.
- Allowed offset and maximum length, if applicable.
- Expiration timestamp.
- Unique token ID for revocation or audit.

The owner peer must validate the token locally or validate a signed token without trusting request fields supplied by the downloader.

### 2.2 Token security

- Use a cryptographically secure random token or a signed token with key rotation.
- Store only a token hash when persistence is required.
- Never log raw tokens.
- Reject expired, reused, malformed, or incorrectly scoped tokens.
- Use constant-time comparison for token hashes.
- Return generic authorization errors.

### 2.3 Phase 2 acceptance criteria

- A requester cannot use a token for another file.
- A requester cannot use a token after expiry.
- A requester cannot use a token for another peer.
- A private file cannot be downloaded without explicit permission.
- Token failures are logged without exposing credentials.

## Phase 3: Encrypted Peer Transfers

### 3.1 Transport security

Use TLS for the direct peer connection. Do not implement custom encryption around raw TCP.

The peer server should use:

- TLS 1.2 or newer, preferably TLS 1.3.
- Certificate or public-key identity for the peer.
- Strong cipher suites supplied by the TLS library.
- Certificate/public-key verification or a tracker-issued peer identity.
- Key and certificate rotation procedures.

The tracker must publish the peer identity needed by the requester to verify the connection. An IP address alone is not an identity.

### 3.2 Encrypted connection sequence

1. Requester authenticates with the tracker.
2. Tracker returns owner peer address, public identity, and transfer token.
3. Requester opens a TCP connection to the owner peer.
4. TLS handshake verifies the owner peer identity.
5. Requester sends the framed transfer request and token over TLS.
6. Owner validates the token and begins the authorized range transfer.
7. Requester verifies the received bytes and closes the connection.

### 3.3 Network fallback

Direct connections may fail because of NAT or firewalls. Add fallback in this order:

1. Direct LAN address.
2. Public peer address if available.
3. Configured relay service carrying the same encrypted transfer protocol.

The tracker should only coordinate the fallback. It should not silently downgrade an encrypted transfer to plaintext.

### 3.4 Phase 3 acceptance criteria

- File bytes are never sent over an unauthenticated plaintext peer connection.
- The requester detects an invalid peer certificate or identity.
- A man-in-the-middle cannot read or alter a transfer undetected.
- TLS errors fail the transfer clearly and do not fall back to plaintext.
- Relay transfers use the same authorization and integrity checks.

## Phase 4: Resumable Downloads

### 4.1 Range requests

Implement the planned `GET file OFFSET` behavior as a stable range request. Prefer an explicit length as well:

```text
GET <file_id> <offset> <length>
```

The server must reject:

- Negative offsets.
- Offsets beyond the file size.
- Lengths larger than the configured maximum.
- Ranges that overflow integer limits.
- Ranges not authorized by the transfer token.

### 4.2 Partial-file format

Store incomplete downloads separately from completed files:

```text
downloads/<file_id>.part
downloads/<file_id>.metadata.json
```

The metadata should include:

- File ID.
- Expected filename.
- Expected size.
- Expected content hash.
- Bytes already written.
- Protocol version.
- Last successful range.

Write data to a temporary file and atomically rename it only after final verification. Do not treat a partial file as available for sharing.

### 4.3 Resume sequence

1. Request metadata from the tracker.
2. Check whether a matching partial file exists.
3. Verify the partial file identity and size.
4. Request the next authorized range from the owner peer.
5. Append the received bytes.
6. Persist progress after each successful chunk.
7. Repeat until the expected size is reached.
8. Verify the complete hash.
9. Atomically rename the completed file into the shared/downloaded location.

### 4.4 Retry behavior

- Retry only failed ranges, not the entire file.
- Use bounded exponential backoff.
- Refresh an expired transfer token before retrying.
- Select another available owner peer when possible.
- Never append bytes unless the response offset matches the requested offset.
- Detect duplicate or overlapping chunks.

### 4.5 Phase 4 acceptance criteria

- An interrupted transfer resumes from the last verified offset.
- A mismatched partial file is discarded or restarted safely.
- A range response cannot write beyond the requested region.
- Retried ranges do not corrupt the output file.
- Completed files are never exposed before verification.

## Phase 5: Hash Verification

### 5.1 Hash commands

Implement:

```text
HASH <file_id>
```

The tracker should return the registered hash. The peer may calculate or confirm the hash from its local file.

The transfer completion response must include:

- File ID.
- Total bytes sent.
- Expected hash.
- Optional per-chunk hashes or a Merkle root for very large files.

### 5.2 Hash workflow

- Calculate the hash when a file is first indexed.
- Recalculate if the file size or modification identity changes.
- Do not trust a client-provided hash without verification.
- Hash the exact bytes that will be served.
- Verify the complete downloaded file before making it available.
- Report integrity failure and delete the invalid temporary output.

### 5.3 Phase 5 acceptance criteria

- A changed source file is detected before transfer.
- A modified transfer fails final verification.
- A successful transfer produces the advertised SHA-256 hash.
- Hash calculation does not require loading the entire file into memory.

## Phase 6: Tracker and Database Changes

Add persistent records for:

### Peer locations

- `peer_id`
- `user_id`
- `address`
- `port`
- `public_key`
- `protocol_version`
- `last_heartbeat`
- `online`

### Transfer authorizations

- `token_id`
- `file_id`
- `requester_peer_id`
- `owner_peer_id`
- `operation`
- `offset`
- `length`
- `expires_at`
- `revoked_at`

### Transfer history

- `transfer_id`
- `file_id`
- `requester_id`
- `owner_id`
- `bytes_transferred`
- `started_at`
- `completed_at`
- `result`
- `failure_reason`

The tracker should remove or mark stale peers after missed heartbeats and must never return stale transfer endpoints without an availability check.

## Phase 7: Testing Plan

### Unit tests

- Safe file path resolution.
- File ID generation and lookup.
- Range parsing and integer-overflow protection.
- Chunk size and transfer-limit enforcement.
- SHA-256 calculation for empty, small, and large files.
- Partial-file metadata validation.
- Token scope and expiration validation.
- TLS identity configuration.

### Integration tests

- Tracker discovery followed by direct peer transfer.
- Public and private file authorization.
- Delete or visibility change while a transfer is pending.
- Owner peer disconnect and alternate peer selection.
- Expired token and invalid token rejection.
- Transfer over TLS with certificate verification.
- Interrupted transfer followed by successful resume.
- Final hash verification after transfer.

### Adversarial tests

- Path traversal and symlink escape.
- Negative and overflowing offsets.
- Oversized range requests.
- Token replay and token substitution.
- Malformed frame headers.
- Slowloris-style partial requests.
- Connection exhaustion.
- Corrupted chunks and truncated transfers.
- Certificate mismatch and plaintext downgrade attempts.

### Load tests

- Multiple concurrent downloads from one peer.
- Multiple peers downloading the same file.
- Large files with frequent resume checkpoints.
- Tracker restart during a transfer.
- Peer restart during a transfer.
- Storage and database growth over long runs.

## Recommended Implementation Order

1. Add `FileManager` with safe path and range operations.
2. Add a local, unauthenticated peer file server for development tests only.
3. Add transfer request framing and bounded range responses.
4. Add file IDs and SHA-256 metadata.
5. Add tracker transfer-token issuance and peer-side token validation.
6. Add TLS and peer identity verification.
7. Add resumable partial-file downloads.
8. Add retry, alternate-peer selection, and relay fallback.
9. Add browser progress, pause, resume, and integrity status.
10. Remove the development-only unauthenticated transfer mode.

## Definition of Done

The remaining direct-transfer milestone is complete when:

- Authenticated peers discover each other through the tracker.
- A peer serves only authorized files from its storage root.
- All direct transfers use verified TLS or an encrypted relay.
- Transfers support bounded ranges and safe resume.
- Completed files pass SHA-256 verification before publication.
- Failed, interrupted, expired, and tampered transfers leave no corrupt shared file.
- Automated unit, integration, security, and load tests pass.
- `changes.md` records each implementation step and validation result.
