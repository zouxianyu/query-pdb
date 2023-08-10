#include <utility>
#include <set>
#include <httplib.h>
#include <cxxopts.hpp>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/daily_file_sink.h>
#include "downloader.h"
#include "pdb_parser.h"

int main(int argc, char *argv[]) {
    cxxopts::Options option_parser("query-pdb", "pdb query server");
    option_parser.add_options()
            ("ip", "ip address", cxxopts::value<std::string>()->default_value("0.0.0.0"))
            ("port", "port", cxxopts::value<uint16_t>()->default_value("8080"))
            ("path", "download path", cxxopts::value<std::string>()->default_value("save"))
            ("server", "download server", cxxopts::value<std::string>()->default_value(
                    "https://msdl.microsoft.com/download/symbols/"))
            ("log", "write log to file", cxxopts::value<bool>()->default_value("false"))
            ("h,help", "print help");

    auto parse_result = option_parser.parse(argc, argv);

    if (parse_result.count("help")) {
        std::cout << option_parser.help() << std::endl;
        return 0;
    }

    const auto ip = parse_result["ip"].as<std::string>();
    const auto port = parse_result["port"].as<uint16_t>();
    const auto download_path = parse_result["path"].as<std::string>();
    const auto download_server = parse_result["server"].as<std::string>();
    const auto log_to_file = parse_result["log"].as<bool>();

    if (log_to_file) {
        spdlog::set_default_logger(spdlog::daily_logger_mt("query-pdb", "server.log"));
    }
    downloader::set_default_pdb_path(download_path);
    downloader::set_default_pdb_server(download_server);

    httplib::Server server;
    server.set_exception_handler([](const auto &req, auto &res, std::exception_ptr ep) {
        std::string content;
        try {
            std::rethrow_exception(ep);
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
    server.Post("/symbol", [](const httplib::Request &req, httplib::Response &res) {
        spdlog::info("symbol request: {}", req.body);
        auto body = nlohmann::json::parse(req.body);
        auto name = body["name"].get<std::string>();
        auto guid = body["guid"].get<std::string>();
        auto age = body["age"].get<uint32_t>();
        auto query = body["query"].get<std::set<std::string>>();

        downloader storage;
        // download pdb
        if (!storage.download(name, guid, age)) {
            throw std::runtime_error("download failed");
        }

        // parse pdb
        pdb_parser parser(storage.get_path(name, guid, age).string());
        nlohmann::json result = parser.get_symbols(query);

        res.set_content(result.dump(), "application/json");
    });

    // example:
    // {
    //     "name": "ntdll.pdb",
    //     "guid": "ABCDEF...",
    //     "age": 1
    //     "query": {
    //         "struct1": {"field1", "field2", "field3"},
    //         "struct2": {"field4", "field5", "field6"},
    //         ...
    //     }
    // }
    server.Post("/struct", [](const httplib::Request &req, httplib::Response &res) {
        spdlog::info("struct request: {}", req.body);
        auto body = nlohmann::json::parse(req.body);
        auto name = body["name"].get<std::string>();
        auto guid = body["guid"].get<std::string>();
        auto age = body["age"].get<uint32_t>();
        auto query = body["query"].get<std::map<std::string, std::set<std::string>>>();

        // download pdb
        downloader storage;
        if (!storage.download(name, guid, age)) {
            throw std::runtime_error("download failed");
        }

        // parse pdb
        pdb_parser parser(storage.get_path(name, guid, age).string());
        std::map<std::string, std::map<std::string, field_info>> result =
                parser.get_struct(query);

        std::map<std::string, std::map<std::string, std::map<std::string, int64_t>>> translate;
        for (const auto &[struct_name, fields]: result) {
            translate[struct_name] = {};
            for (const auto &[field_name, field]: fields) {
                translate[struct_name][field_name] = field.to_map();
            }
        }

        res.set_content(nlohmann::json(translate).dump(), "application/json");
    });

    // example:
    // {
    //     "name": "ntdll.pdb",
    //     "guid": "ABCDEF...",
    //     "age": 1
    //     "query": {
    //         "enum1": {"name1", "name2", "name3"},
    //         "enum2": {"name4", "name5", "name6"},
    //         ...
    //     }
    // }
    server.Post("/enum", [](const httplib::Request &req, httplib::Response &res) {
        spdlog::info("enum request: {}", req.body);
        auto body = nlohmann::json::parse(req.body);
        auto name = body["name"].get<std::string>();
        auto guid = body["guid"].get<std::string>();
        auto age = body["age"].get<uint32_t>();
        auto query = body["query"].get<std::map<std::string, std::set<std::string>>>();

        // download pdb
        downloader storage;
        if (!storage.download(name, guid, age)) {
            throw std::runtime_error("download failed");
        }

        // parse pdb
        pdb_parser parser(storage.get_path(name, guid, age).string());
        nlohmann::json result = parser.get_enum(query);

        res.set_content(result.dump(), "application/json");
    });

    server.listen(ip, port);
    return 0;
}
