#include "utils/HttpServer.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

#include "utils/Json.h"
#include "utils/StringUtils.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle = SOCKET;
static const SocketHandle kInvalidSocket = INVALID_SOCKET;
static void closeSocket(SocketHandle s) { closesocket(s); }
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
using SocketHandle = int;
static const SocketHandle kInvalidSocket = -1;
static void closeSocket(SocketHandle s) { ::close(s); }
#endif
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

namespace fs = std::filesystem;

namespace {
constexpr size_t kMaxHeaderBytes = 16 * 1024;
constexpr size_t kMaxBodyBytes = 1024 * 1024;

const char* statusText(int status) {
    switch (status) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 413: return "Payload Too Large";
        case 431: return "Request Header Fields Too Large";
        default: return status >= 500 ? "Internal Server Error" : "OK";
    }
}

std::string mimeType(const fs::path& path) {
    const std::string ext = StringUtils::toLower(path.extension().string());
    if (ext == ".html") return "text/html; charset=utf-8";
    if (ext == ".css") return "text/css; charset=utf-8";
    if (ext == ".js") return "application/javascript; charset=utf-8";
    if (ext == ".json") return "application/json; charset=utf-8";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".txt" || ext == ".csv") return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

HttpResponse errorResponse(int status, const std::string& message) {
    Json body = Json::object();
    body.set("success", false).set("message", message);
    return {status, "application/json; charset=utf-8", body.dump()};
}

std::vector<std::string> splitPath(const std::string& path) {
    std::vector<std::string> parts;
    for (const std::string& piece : StringUtils::split(path, '/'))
        if (!piece.empty()) parts.push_back(piece);
    return parts;
}

void parseQuery(const std::string& text, std::map<std::string, std::string>& out) {
    for (const std::string& pair : StringUtils::split(text, '&')) {
        if (pair.empty()) continue;
        const auto eq = pair.find('=');
        const std::string key = Http::urlDecode(pair.substr(0, eq), true);
        out[key] = eq == std::string::npos ? "" : Http::urlDecode(pair.substr(eq + 1), true);
    }
}

bool parseRequest(const std::string& head, HttpRequest& request) {
    std::istringstream stream(head);
    std::string line, target, version;
    if (!std::getline(stream, line)) return false;
    std::istringstream first(line);
    if (!(first >> request.method >> target >> version)) return false;
    const auto q = target.find('?');
    request.path = Http::urlDecode(target.substr(0, q), false);
    if (q != std::string::npos) parseQuery(target.substr(q + 1), request.query);
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        request.headers[StringUtils::toLower(line.substr(0, colon))] = StringUtils::trim(line.substr(colon + 1));
    }
    return true;
}

bool sendAll(SocketHandle socket, const std::string& data) {
    size_t sent = 0;
    while (sent < data.size()) {
        const auto n = ::send(socket, data.data() + sent, static_cast<int>(data.size() - sent), MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}
}  // namespace

namespace Http {
std::string urlDecode(const std::string& text, bool plusIsSpace) {
    std::string out;
    for (size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '%' && i + 2 < text.size() && std::isxdigit(static_cast<unsigned char>(text[i + 1])) &&
            std::isxdigit(static_cast<unsigned char>(text[i + 2]))) {
            out += static_cast<char>(std::stoi(text.substr(i + 1, 2), nullptr, 16));
            i += 2;
        } else if (text[i] == '+' && plusIsSpace) {
            out += ' ';
        } else {
            out += text[i];
        }
    }
    return out;
}
}  // namespace Http

std::string HttpRequest::get(const std::string& key) const {
    auto it = query.find(key);
    return it == query.end() ? std::string() : it->second;
}
std::string HttpRequest::param(const std::string& name) const {
    auto it = params.find(name);
    return it == params.end() ? std::string() : it->second;
}

void Router::add(const std::string& method, const std::string& pattern, Handler handler) {
    routes_.push_back({method, splitPath(pattern), std::move(handler)});
}

HttpResponse Router::handle(HttpRequest request) const {
    const std::vector<std::string> parts = splitPath(request.path);
    bool pathMatched = false;
    for (const Route& route : routes_) {
        if (route.segments.size() != parts.size()) continue;
        std::map<std::string, std::string> captured;
        bool match = true;
        for (size_t i = 0; i < parts.size() && match; ++i) {
            const std::string& seg = route.segments[i];
            if (seg.size() > 2 && seg.front() == '{' && seg.back() == '}') captured[seg.substr(1, seg.size() - 2)] = parts[i];
            else match = (seg == parts[i]);
        }
        if (!match) continue;
        pathMatched = true;
        if (route.method != request.method) continue;
        request.params = captured;
        return route.handler(request);
    }
    return pathMatched ? errorResponse(405, "Method not allowed for this endpoint.")
                       : errorResponse(404, "Endpoint not found: " + request.path);
}

HttpServer::HttpServer(std::string host, int port, fs::path staticRoot, const Router& router)
    : host_(std::move(host)), port_(port), staticRoot_(std::move(staticRoot)), router_(router) {
    std::error_code ec;
    const fs::path canonical = fs::weakly_canonical(staticRoot_, ec);
    if (!ec) staticRoot_ = canonical;
}

HttpResponse HttpServer::serveStatic(const HttpRequest& request) const {
    if (request.method != "GET" && request.method != "HEAD") return errorResponse(405, "Method not allowed.");
    std::string relative = request.path;
    while (!relative.empty() && relative.front() == '/') relative.erase(relative.begin());
    if (relative.empty()) relative = "index.html";
    std::error_code ec;
    fs::path full = fs::weakly_canonical(staticRoot_ / relative, ec);
    const std::string root = staticRoot_.generic_string(), candidate = full.generic_string();
    if (ec || candidate.compare(0, root.size(), root) != 0) return errorResponse(403, "Forbidden.");  // blocks ../ traversal
    if (fs::is_directory(full, ec)) full /= "index.html";
    std::ifstream file(full, std::ios::binary);
    if (!file) return errorResponse(404, "File not found: " + request.path);
    std::ostringstream content;
    content << file.rdbuf();
    return {200, mimeType(full), content.str()};
}

void HttpServer::serveConnection(long long handle) {
    const SocketHandle client = static_cast<SocketHandle>(handle);
#ifdef _WIN32
    DWORD timeoutMs = 5000;
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof timeoutMs);
#else
    timeval timeout{5, 0};
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
#endif
    std::string data;
    char buffer[4096];
    size_t headerEnd = std::string::npos;
    HttpResponse response;
    bool haveRequest = false;
    HttpRequest request;

    while (headerEnd == std::string::npos) {
        const auto n = ::recv(client, buffer, sizeof buffer, 0);
        if (n <= 0) { closeSocket(client); return; }
        data.append(buffer, static_cast<size_t>(n));
        headerEnd = data.find("\r\n\r\n");
        if (headerEnd == std::string::npos && data.size() > kMaxHeaderBytes) {
            response = errorResponse(431, "Request headers too large.");
            sendAll(client, "HTTP/1.1 431 Request Header Fields Too Large\r\nConnection: close\r\nContent-Length: 0\r\n\r\n");
            closeSocket(client);
            return;
        }
    }

    if (!parseRequest(data.substr(0, headerEnd), request)) {
        response = errorResponse(400, "Malformed HTTP request.");
    } else {
        size_t contentLength = 0;
        auto it = request.headers.find("content-length");
        if (it != request.headers.end()) contentLength = static_cast<size_t>(std::strtoull(it->second.c_str(), nullptr, 10));
        if (contentLength > kMaxBodyBytes) {
            response = errorResponse(413, "Request body too large.");
        } else {
            std::string body = data.substr(headerEnd + 4);
            while (body.size() < contentLength) {
                const auto n = ::recv(client, buffer, sizeof buffer, 0);
                if (n <= 0) break;
                body.append(buffer, static_cast<size_t>(n));
            }
            request.body = body.substr(0, contentLength);
            haveRequest = true;
        }
    }

    if (haveRequest) {
        try {
            if (request.method == "OPTIONS") {
                response = {204, "text/plain", ""};
            } else if (request.path.rfind("/api", 0) == 0) {
                std::lock_guard<std::mutex> lock(handlerMutex_);  // one business operation at a time
                response = router_.handle(request);
            } else {
                response = serveStatic(request);
            }
        } catch (const std::exception& e) {
            response = errorResponse(500, std::string("Internal server error: ") + e.what());
        } catch (...) {
            response = errorResponse(500, "Internal server error.");
        }
    }

    std::ostringstream out;
    out << "HTTP/1.1 " << response.status << ' ' << statusText(response.status) << "\r\n"
        << "Content-Type: " << response.contentType << "\r\n"
        << "Content-Length: " << response.body.size() << "\r\n"
        << "Cache-Control: no-cache\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"
        << "Access-Control-Allow-Headers: Content-Type\r\n"
        << "Connection: close\r\n\r\n";
    if (request.method != "HEAD") out << response.body;
    sendAll(client, out.str());
    closeSocket(client);
}

void HttpServer::run() {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) throw std::runtime_error("WSAStartup failed");
#else
    signal(SIGPIPE, SIG_IGN);
#endif
    const SocketHandle server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server == kInvalidSocket) throw std::runtime_error("Cannot create socket");
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse), sizeof reuse);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<unsigned short>(port_));
    if (inet_pton(AF_INET, host_.c_str(), &address.sin_addr) != 1) throw std::runtime_error("Invalid host address: " + host_);
    if (::bind(server, reinterpret_cast<sockaddr*>(&address), sizeof address) != 0)
        throw std::runtime_error("Cannot bind to " + host_ + ":" + std::to_string(port_) + " (is the port already in use?)");
    if (::listen(server, 64) != 0) throw std::runtime_error("listen() failed");

    std::cout << "Smart Library listening on http://" << host_ << ":" << port_ << "  (Ctrl+C to stop)" << std::endl;
    while (true) {
        const SocketHandle client = ::accept(server, nullptr, nullptr);
        if (client == kInvalidSocket) continue;
        std::thread(&HttpServer::serveConnection, this, static_cast<long long>(client)).detach();
    }
}
