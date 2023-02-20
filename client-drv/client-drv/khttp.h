#ifndef QPDB_DRIVER_KHTTP_H
#define QPDB_DRIVER_KHTTP_H

#include <string>
#include <memory>
#include "berkeley.h"
#include "ksocket.h"
#include "urlparser.h"
#include "httpresponseparser.h"

class khttp {
public:
    explicit khttp(const std::string &server);

    httpparser::Response get(const std::string &path);

    httpparser::Response post(const std::string &path, const std::string &content);

    httpparser::Response put(const std::string &path, const std::string &content);

    httpparser::Response del(const std::string &path);

private:
    class ks_initializer {
    public:
        ks_initializer() : valid_(NT_SUCCESS(KsInitialize())) {}

        ~ks_initializer() { if (valid_) KsDestroy(); }

        bool valid() const { return valid_; }

    private:
        bool valid_;
    };

    struct header_config {
        std::string user_agent;
    };

    static ks_initializer ksinit_;
    httpparser::UrlParser url_parser_;
    header_config header_config_;

    std::string build_request(
            const std::string &method,
            const std::string &path,
            const std::string &content
    );

    httpparser::Response http_impl(
            const std::string &method,
            const std::string &path,
            const std::string &content
    );

};

#endif //QPDB_DRIVER_KHTTP_H
