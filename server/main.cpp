#include <json.hpp>
#include <utility>
#include <httplib.h>
#include <cmdline.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "downloader.h"

int main(int argc, char *argv[]) {
    cmdline::parser parser;
    parser.add<std::string>("ip", 'i', "ip address", false, "0.0.0.0");
    parser.add<uint16_t>("port", 'p', "port", false, 8080);
    parser.add<std::string>("path", 'd', "download path", false, "save");
    parser.add<std::string>("server", 's', "download server", false,
                            "http://msdl.microsoft.com/download/symbols/");
    parser.add<bool>("log", 'l', "write log to file", false, false);
    parser.parse_check(argc, argv);

    const auto ip = parser.get<std::string>("ip");
    const auto port = parser.get<uint16_t>("port");
    const auto download_path = parser.get<std::string>("path");
    const auto download_server = parser.get<std::string>("server");
    const auto log_to_file = parser.get<bool>("log");

    if (log_to_file) {
        spdlog::set_default_logger(spdlog::daily_logger_mt("query-pdb", "server.log"));
    }

    downloader storage(download_path, download_server);
    if (!storage.valid()) {
        spdlog::error("exit due to downloader invalid");
        return 1;
    }

    httplib::Server server;
    server.set_exception_handler([](const auto &req, auto &res, std::exception_ptr ep) {
        std::string content;
        try {
            std::rethrow_exception(std::move(ep));
        } catch (std::exception &e) {
            content = e.what();
        } catch (...) {
            content = "Unknown Exception";
        }
        res.set_content(content, "plain/text");
        res.status = 500;

        spdlog::error("exception: {}", content);
    });

    // example:
    // {
    //     "name": "ntdll.pdb",
    //     "guid": "ABCDEF...",
    //     "age": 1
    //     "query": [
    //         "Name1",
    //         "Name2",
    //         ...
    //     ]
    // }
    server.Post("/global", [&storage](const httplib::Request &req, httplib::Response &res) {
        spdlog::info("request: {}", req.body);
        auto body = nlohmann::json::parse(req.body);
        auto name = body["name"].get<std::string>();
        auto guid = body["guid"].get<std::string>();
        auto age = body["age"].get<uint32_t>();
        auto query = body["query"].get<std::vector<std::string>>();

        // download pdb
        if (!storage.download(name, guid, age)) {
            throw std::runtime_error("download failed");
        }

        // parse pdb
        res.set_content("ok", "text/plain");
    });

    server.listen(ip, port);
    return 0;
}
