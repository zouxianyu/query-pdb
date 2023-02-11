#include <utility>
#include <httplib.h>
#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include "downloader.h"

int main(int argc, char *argv[]) {
    cxxopts::Options option_parser("query-pdb", "pdb query server");
    option_parser.add_options()
            ("ip", "ip address", cxxopts::value<std::string>()->default_value("0.0.0.0"))
            ("port", "port", cxxopts::value<uint16_t>()->default_value("8080"))
            ("path", "download path", cxxopts::value<std::string>()->default_value("save"))
            ("server", "download server", cxxopts::value<std::string>()->default_value(
                    "http://msdl.microsoft.com/download/symbols/"))
            ("log", "write log to file", cxxopts::value<bool>()->default_value("false"));

    auto parse_result = option_parser.parse(argc, argv);

    const auto ip = parse_result["ip"].as<std::string>();
    const auto port = parse_result["port"].as<uint16_t>();
    const auto download_path = parse_result["path"].as<std::string>();
    const auto download_server = parse_result["server"].as<std::string>();
    const auto log_to_file = parse_result["log"].as<bool>();

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
