#include <memory>
#include "khttp.h"

class socket_guard {
public:
    explicit socket_guard(int fd) : fd_(fd) {}

    int get() const {
        return fd_;
    }

    ~socket_guard() {
        if (fd_ != -1) {
            closesocket(fd_);
        }
    }

private:
    int fd_;
};

struct addrinfo_deleter {
    void operator()(addrinfo *p) {
        freeaddrinfo(p);
    }
};

using addrinfo_guard = std::unique_ptr<addrinfo, addrinfo_deleter>;

khttp::ks_initializer khttp::ksinit_;

khttp::khttp(const std::string &server)
        : url_parser_(server),
          header_config_({"Windows WSK Client"}) {

    if (!ksinit_.valid()) {
        KdPrint(("Failed to initialize ksocket\n"));
    }

    if (!url_parser_.isValid()) {
        KdPrint(("Failed to parse url: %s\n", server.c_str()));
    } else {
        KdPrint(("host: %s, port: %s\n",
                url_parser_.hostname().c_str(),
                url_parser_.port().c_str()));
    }
}

httpparser::Response khttp::get(const std::string &path) {
    return http_impl("GET", path, "");
}

httpparser::Response khttp::post(const std::string &path, const std::string &content) {
    return http_impl("POST", path, content);
}

httpparser::Response khttp::put(const std::string &path, const std::string &content) {
    return http_impl("PUT", path, content);
}

httpparser::Response khttp::del(const std::string &path) {
    return http_impl("DELETE", path, "");
}

std::string khttp::build_request(
        const std::string &method,
        const std::string &path,
        const std::string &content
) {
    std::string s;
    s += method + " " + path + " HTTP/1.1\r\n";
    s += "Host: " + url_parser_.hostname() + "\r\n";
    s += "User-Agent: " + header_config_.user_agent + "\r\n";
    s += "Accept: */*\r\n";
    s += "Connection: close\r\n";
    s += "Content-Length: " + std::to_string(content.length()) + "\r\n";
    s += "\r\n";
    s += content;
    return s;
}

httpparser::Response khttp::http_impl(
        const std::string &method,
        const std::string &path,
        const std::string &content
) {
    // sanity check
    if (!ksinit_.valid() ||
        !url_parser_.isValid() ||
        url_parser_.scheme() != "http" ||
        url_parser_.hostname().empty() ||
        method.empty() ||
        path.empty()) {
        return {};
    }

    struct addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    std::string port = url_parser_.port().empty() ? "80" : url_parser_.port();

    addrinfo *addr_raw = nullptr;
    addrinfo_guard addr;
    if (getaddrinfo(
            url_parser_.hostname().c_str(),
            port.c_str(),
            &hints,
            &addr_raw
    ) < 0) {
        addr.reset(addr_raw);
        KdPrint(("Failed to get address info: %s\n", url_parser_.hostname().c_str()));
        return {};
    }
    addr.reset(addr_raw);
    KdPrint(("Got address info: %s\n", url_parser_.hostname().c_str()));

    socket_guard sockfd(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (sockfd.get() < 0) {
        KdPrint(("Failed to create socket\n"));
        return {};
    }
    KdPrint(("Created socket: %d\n", sockfd.get()));

    if (connect(sockfd.get(), addr->ai_addr, addr->ai_addrlen) < 0) {
        KdPrint(("Failed to connect to server: %s\n", url_parser_.hostname().c_str()));
        return {};
    }
    KdPrint(("Connected to server: %s\n", url_parser_.hostname().c_str()));

    std::string request = build_request(method, path, content);
    KdPrint(("Request: \n%s\n", request.c_str()));

    while (true) {
        int sent = send(sockfd.get(), request.c_str(), request.length(), 0);
        if (sent < 0) {
            KdPrint(("Failed to send request\n"));
            return {};
        }
        if (static_cast<size_t>(sent) == request.length()) {
            break;
        }
        request = request.substr(sent);
    }
    KdPrint(("Sent request\n"));

    httpparser::HttpResponseParser parser;
    httpparser::HttpResponseParser::ParseResult parse_result = 
        httpparser::HttpResponseParser::ParsingError;
    httpparser::Response response;

    while (true) {
        char buf[512];
        int received = recv(sockfd.get(), buf, sizeof(buf), 0);
        if (received < 0) {
            KdPrint(("Failed to receive response\n"));
            return {};
        }
        if (received == 0) {
            break;
        }
        parse_result = parser.parse(response, buf, buf + received);
    }
    KdPrint(("Received response\n"));

    if (parse_result != httpparser::HttpResponseParser::ParsingCompleted) {
        KdPrint(("Failed to parse response\n"));
        return {};
    }
    KdPrint(("Parsed response\n"));

    return response;
}
