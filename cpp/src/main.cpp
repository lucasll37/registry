/**
 * main.cpp — MinIO / AIStor C++ PoC
 *
 * Mirrors the behaviour of python/bucket_demo.py:
 *   1. demo_shared()   — upload, download, list, delete on bucket "shared"
 *   2. demo_readonly() — list + read on "readonly"; assert write is denied
 *   3. demo_personal() — CRUD under users/<username>/; assert cross-user
 *                        access is denied
 *
 * Credentials are read from environment variables (same .env the Python
 * scripts use — source it or export before running):
 *
 *   MINIO_ENDPOINT        e.g. localhost:9000
 *   MINIO_SVC_ACCESS_KEY  service-account access key
 *   MINIO_SVC_SECRET_KEY  service-account secret key
 *   MINIO_SVC_USERNAME    username (used as personal-folder prefix)
 *   MINIO_SECURE          "true" | "false"  (default: false)
 *
 * Build (after `conan install`):
 *   meson setup build --native-file build/conan_meson_native.ini
 *   meson compile -C build
 *   ./build/minio_poc
 */

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/stream/PreallocatedStreamBuf.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/PutObjectRequest.h>

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

// ── ANSI colours ─────────────────────────────────────────────────────────────

namespace colour {
constexpr const char* reset  = "\033[0m";
constexpr const char* red    = "\033[0;31m";
constexpr const char* green  = "\033[0;32m";
constexpr const char* yellow = "\033[1;33m";
constexpr const char* blue   = "\033[0;34m";
} // namespace colour

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::string env_or(const char* name, const char* fallback = "")
{
    const char* v = std::getenv(name);
    return v ? std::string(v) : std::string(fallback);
}

static void section(const std::string& title)
{
    std::cout << "\n" << colour::blue
              << "── " << title << " ──"
              << colour::reset << "\n";
}

// ── S3 operations ─────────────────────────────────────────────────────────────

/**
 * Upload a plain-text / JSON string as an S3 object.
 */
static void upload_text(const Aws::S3::S3Client& s3,
                         const std::string& bucket,
                         const std::string& key,
                         const std::string& body)
{
    auto stream = Aws::MakeShared<Aws::StringStream>("upload");
    *stream << body;

    Aws::S3::Model::PutObjectRequest req;
    req.SetBucket(bucket);
    req.SetKey(key);
    req.SetContentType("application/json");
    req.SetBody(stream);

    auto outcome = s3.PutObject(req);
    if (!outcome.IsSuccess()) {
        throw std::runtime_error(
            "PutObject failed [" + key + "]: " +
            outcome.GetError().GetMessage());
    }
    std::cout << colour::green << "  ✓ upload  → s3://" << bucket << "/" << key
              << colour::reset << "\n";
}

/**
 * Download an S3 object and return its body as a string.
 */
static std::string download_text(const Aws::S3::S3Client& s3,
                                  const std::string& bucket,
                                  const std::string& key)
{
    Aws::S3::Model::GetObjectRequest req;
    req.SetBucket(bucket);
    req.SetKey(key);

    auto outcome = s3.GetObject(req);
    if (!outcome.IsSuccess()) {
        throw std::runtime_error(
            "GetObject failed [" + key + "]: " +
            outcome.GetError().GetMessage());
    }

    std::ostringstream oss;
    oss << outcome.GetResult().GetBody().rdbuf();
    std::cout << colour::green << "  ✓ download ← s3://" << bucket << "/" << key
              << colour::reset << "\n";
    return oss.str();
}

/**
 * List all keys in a bucket under a given prefix.
 */
static std::vector<std::string> list_objects(const Aws::S3::S3Client& s3,
                                              const std::string& bucket,
                                              const std::string& prefix = "")
{
    Aws::S3::Model::ListObjectsV2Request req;
    req.SetBucket(bucket);
    if (!prefix.empty()) req.SetPrefix(prefix);

    std::vector<std::string> keys;
    bool truncated = true;

    while (truncated) {
        auto outcome = s3.ListObjectsV2(req);
        if (!outcome.IsSuccess()) {
            throw std::runtime_error(
                "ListObjectsV2 failed [" + bucket + "/" + prefix + "]: " +
                outcome.GetError().GetMessage());
        }
        const auto& result = outcome.GetResult();
        for (const auto& obj : result.GetContents()) {
            keys.emplace_back(obj.GetKey());
        }
        truncated = result.GetIsTruncated();
        if (truncated) req.SetContinuationToken(result.GetNextContinuationToken());
    }
    return keys;
}

/**
 * Delete a single S3 object.
 */
static void delete_object(const Aws::S3::S3Client& s3,
                           const std::string& bucket,
                           const std::string& key)
{
    Aws::S3::Model::DeleteObjectRequest req;
    req.SetBucket(bucket);
    req.SetKey(key);

    auto outcome = s3.DeleteObject(req);
    if (!outcome.IsSuccess()) {
        throw std::runtime_error(
            "DeleteObject failed [" + key + "]: " +
            outcome.GetError().GetMessage());
    }
    std::cout << colour::green << "  ✓ delete   → s3://" << bucket << "/" << key
              << colour::reset << "\n";
}

// ── Demo sections ─────────────────────────────────────────────────────────────

static void demo_shared(const Aws::S3::S3Client& s3)
{
    section("Bucket: shared (leitura e escrita)");

    const std::string key = "experimento-01/config.json";
    const std::string body = R"({
  "modelo": "mlp",
  "epocas": 100,
  "lr": 0.001
})";

    upload_text(s3, "shared", key, body);

    std::string content = download_text(s3, "shared", key);
    std::cout << "  conteúdo:\n" << content << "\n";

    auto keys = list_objects(s3, "shared", "experimento-01/");
    std::cout << "  objetos: ";
    for (const auto& k : keys) std::cout << k << " ";
    std::cout << "\n";

    delete_object(s3, "shared", key);
}

static void demo_readonly(const Aws::S3::S3Client& s3)
{
    section("Bucket: readonly (somente leitura)");

    auto keys = list_objects(s3, "readonly");
    if (!keys.empty()) {
        std::string content = download_text(s3, "readonly", keys[0]);
        std::cout << "  conteúdo:\n" << content << "\n";
    } else {
        std::cout << "  (vazio — aguardando conteúdo do admin)\n";
    }

    // Assert that write is denied
    try {
        upload_text(s3, "readonly", "tentativa.json", R"({"erro":"esperado"})");
        std::cerr << colour::yellow << "  ⚠ escrita não foi bloqueada — verifique a policy!"
                  << colour::reset << "\n";
    } catch (const std::runtime_error& e) {
        std::cout << colour::green
                  << "  ✓ escrita bloqueada como esperado: " << e.what()
                  << colour::reset << "\n";
    }
}

static void demo_personal(const Aws::S3::S3Client& s3, const std::string& username)
{
    section("Bucket: users/" + username + "/ (pasta pessoal)");

    const std::string key = username + "/resultado-42.json";
    const std::string body = R"({
  "acuracia": 0.97,
  "loss": 0.032,
  "epocas": 50
})";

    upload_text(s3, "users", key, body);

    std::string content = download_text(s3, "users", key);
    std::cout << "  conteúdo:\n" << content << "\n";

    auto keys = list_objects(s3, "users", username + "/");
    std::cout << "  objetos: ";
    for (const auto& k : keys) std::cout << k << " ";
    std::cout << "\n";

    delete_object(s3, "users", key);

    // Assert cross-user access is denied
    try {
        list_objects(s3, "users", "outro-usuario/");
        std::cerr << colour::yellow
                  << "  ⚠ acesso indevido a pasta alheia!"
                  << colour::reset << "\n";
    } catch (const std::runtime_error& e) {
        std::cout << colour::green
                  << "  ✓ acesso a pasta alheia bloqueado: " << e.what()
                  << colour::reset << "\n";
    }
}

// ── main ──────────────────────────────────────────────────────────────────────

int main()
{
    // Read credentials from environment
    const std::string endpoint   = env_or("MINIO_ENDPOINT",        "localhost:9000");
    const std::string access_key = env_or("MINIO_SVC_ACCESS_KEY");
    const std::string secret_key = env_or("MINIO_SVC_SECRET_KEY");
    const std::string username   = env_or("MINIO_SVC_USERNAME");
    const bool        secure     = env_or("MINIO_SECURE", "false") == "true";

    if (access_key.empty() || secret_key.empty() || username.empty()) {
        std::cerr << colour::red
                  << "Erro: defina MINIO_SVC_ACCESS_KEY, MINIO_SVC_SECRET_KEY "
                     "e MINIO_SVC_USERNAME no ambiente.\n"
                  << colour::reset;
        return 1;
    }

    std::cout << colour::blue
              << "=== MinIO C++ PoC ===\n"
              << colour::reset;
    std::cout << "  Endpoint : " << (secure ? "https" : "http") << "://" << endpoint << "\n";
    std::cout << "  Username : " << username << "\n";

    // Initialise the AWS SDK
    Aws::SDKOptions sdk_opts;
    sdk_opts.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Warn;
    Aws::InitAPI(sdk_opts);

    int exit_code = 0;

    {
        // Client configuration — path-style addressing is mandatory for MinIO
        Aws::Client::ClientConfiguration cfg;
        cfg.endpointOverride        = endpoint;
        cfg.scheme                  = secure ? Aws::Http::Scheme::HTTPS
                                              : Aws::Http::Scheme::HTTP;
        cfg.verifySSL               = secure;  // disable cert check for self-signed
        cfg.region                  = "us-east-1"; // MinIO ignores region but SDK requires one

        Aws::Auth::AWSCredentials creds(access_key, secret_key);

        // useVirtualAddressing = false → path-style (required for MinIO)
        Aws::S3::S3Client s3(creds, nullptr, cfg,
                             Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never,
                             /*useVirtualAddressing=*/false);

        try {
            demo_shared(s3);
            demo_readonly(s3);
            demo_personal(s3, username);

            std::cout << "\n" << colour::green << "✓ PoC concluído com sucesso."
                      << colour::reset << "\n";
        } catch (const std::exception& ex) {
            std::cerr << colour::red << "\n✗ Erro inesperado: " << ex.what()
                      << colour::reset << "\n";
            exit_code = 1;
        }
    }

    Aws::ShutdownAPI(sdk_opts);
    return exit_code;
}