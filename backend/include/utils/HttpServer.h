#pragma once
// Minimal HTTP/1.1 transport layer (sockets only). It knows NOTHING about the
// library domain: it parses requests, dispatches them to a Router and writes
// responses. Business logic stays in the C++ services.
#include <filesystem>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::map<std::string, std::string> query;    // ?a=b
    std::map<std::string, std::string> params;   // {id} captured from the route pattern
    std::map<std::string, std::string> headers;  // lower-case names

    std::string get(const std::string& key) const;    // query parameter or ""
    bool has(const std::string& key) const { return query.count(key) > 0; }
    std::string param(const std::string& name) const;
};

struct HttpResponse {
    int status = 200;
    std::string contentType = "application/json; charset=utf-8";
    std::string body;
};

using Handler = std::function<HttpResponse(const HttpRequest&)>;

class Router {
public:
    void add(const std::string& method, const std::string& pattern, Handler handler);
    HttpResponse handle(HttpRequest request) const;  // 404 / 405 answered as JSON error envelopes

private:
    struct Route { std::string method; std::vector<std::string> segments; Handler handler; };
    std::vector<Route> routes_;
};

class HttpServer {
public:
    HttpServer(std::string host, int port, std::filesystem::path staticRoot, const Router& router);
    void run();  // blocks; one thread per connection, handlers are serialized by a mutex

private:
    void serveConnection(long long socketHandle);
    HttpResponse serveStatic(const HttpRequest& request) const;

    std::string host_;
    int port_;
    std::filesystem::path staticRoot_;
    const Router& router_;
    std::mutex handlerMutex_;
};

namespace Http {
std::string urlDecode(const std::string& text, bool plusIsSpace);
}
