#include "tls_context.h"

#include <cstdio>
#include <iostream>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>

namespace p2p {

namespace {

bool generateSelfSignedCertificate(const std::filesystem::path& certPath,
                                    const std::filesystem::path& keyPath) {
    EVP_PKEY* pkey = EVP_PKEY_Q_keygen(nullptr, nullptr, "EC", "P-256");
    if (pkey == nullptr) {
        return false;
    }

    X509* cert = X509_new();
    if (cert == nullptr) {
        EVP_PKEY_free(pkey);
        return false;
    }

    X509_set_version(cert, 2);
    ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
    X509_gmtime_adj(X509_get_notBefore(cert), 0);
    X509_gmtime_adj(X509_get_notAfter(cert), 60L * 60L * 24L * 365L);
    X509_set_pubkey(cert, pkey);

    X509_NAME* name = X509_get_subject_name(cert);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC,
                                reinterpret_cast<const unsigned char*>("p2p-peer"), -1, -1, 0);
    X509_set_issuer_name(cert, name);

    const bool signedOk = X509_sign(cert, pkey, EVP_sha256()) > 0;

    bool wroteKey = false;
    bool wroteCert = false;

    if (signedOk) {
        if (FILE* keyFile = std::fopen(keyPath.string().c_str(), "wb")) {
            wroteKey = PEM_write_PrivateKey(keyFile, pkey, nullptr, nullptr, 0, nullptr, nullptr) == 1;
            std::fclose(keyFile);
        }
        if (FILE* certFile = std::fopen(certPath.string().c_str(), "wb")) {
            wroteCert = PEM_write_X509(certFile, cert) == 1;
            std::fclose(certFile);
        }
    }

    X509_free(cert);
    EVP_PKEY_free(pkey);
    return signedOk && wroteKey && wroteCert;
}

} // namespace

TlsServerContext::TlsServerContext(const std::filesystem::path& certDirectory) : ctx_(nullptr) {
    std::error_code errorCode;
    std::filesystem::create_directories(certDirectory, errorCode);

    const auto certPath = certDirectory / "peer_cert.pem";
    const auto keyPath = certDirectory / "peer_key.pem";

    if (!std::filesystem::exists(certPath) || !std::filesystem::exists(keyPath)) {
        if (!generateSelfSignedCertificate(certPath, keyPath)) {
            std::cerr << "Failed to generate self-signed TLS certificate" << std::endl;
            return;
        }
    }

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (ctx == nullptr) {
        return;
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    if (SSL_CTX_use_certificate_file(ctx, certPath.string().c_str(), SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_use_PrivateKey_file(ctx, keyPath.string().c_str(), SSL_FILETYPE_PEM) != 1 ||
        SSL_CTX_check_private_key(ctx) != 1) {
        std::cerr << "Failed to load TLS certificate or key" << std::endl;
        SSL_CTX_free(ctx);
        return;
    }

    ctx_ = ctx;
}

TlsServerContext::~TlsServerContext() {
    if (ctx_ != nullptr) {
        SSL_CTX_free(ctx_);
    }
}

} // namespace p2p
