#ifndef TLS_CONTEXT_H
#define TLS_CONTEXT_H

#include <filesystem>

#include <openssl/ssl.h>

namespace p2p {

// Owns a server-side SSL_CTX backed by a self-signed certificate that is
// generated on first use and reused afterward. Peer transfers are never
// served in plaintext once this context is constructed successfully.
class TlsServerContext {
public:
    explicit TlsServerContext(const std::filesystem::path& certDirectory);
    ~TlsServerContext();

    TlsServerContext(const TlsServerContext&) = delete;
    TlsServerContext& operator=(const TlsServerContext&) = delete;

    bool isValid() const { return ctx_ != nullptr; }
    SSL_CTX* handle() const { return ctx_; }

private:
    SSL_CTX* ctx_;
};

} // namespace p2p

#endif // TLS_CONTEXT_H
