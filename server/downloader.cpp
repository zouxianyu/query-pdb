#include <sstream>
#include <filesystem>
#include <regex>
#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>
#include <spdlog/spdlog.h>
#include "downloader.h"


std::string downloader::pdb_path_="";
std::pair<std::string,std::string> downloader::server_split_= std::make_pair("","");
std::string downloader::msdl_server_="";

void downloader::set_default_pdb_path(std::string dir_path) {
    downloader::pdb_path_=dir_path;
    spdlog::info("set_default_pdb_path: {}", dir_path);
}

void downloader::set_default_pdb_server(std::string msdl_server) {
    spdlog::info("set_default_pdb_server: {}", msdl_server);
    std::regex regex(R"(^((?:(?:http|https):\/\/)?[^\/]+)(\/.*)$)");
    std::smatch match;
    if (!std::regex_match(msdl_server, match, regex)) {
        spdlog::info("invaild server url: {}",msdl_server);
        return;
    }
    downloader::server_split_= std::make_pair(match[1].str(), match[2].str());
    downloader::msdl_server_=msdl_server;
}

downloader::downloader(){

}

bool downloader::download(const std::string &name, const std::string &guid, uint32_t age) {
    std::string relative_path = get_relative_path_str(name, guid, age);
    auto path = get_path(name, guid, age);
    spdlog::info("lookup pdb, path: {}", relative_path);

    if (std::filesystem::exists(path)) {
        spdlog::info("pdb already exists, path: {}", relative_path);
        return true;
    }

    return download_impl(relative_path);
}

std::string downloader::get_relative_path_str(const std::string &name, const std::string &guid, uint32_t age) {
    std::stringstream ss;
    ss << std::hex << std::uppercase;
    ss << name << '/' << guid << age << '/' << name;
    return ss.str();
}

std::filesystem::path downloader::get_path(const std::string &name, const std::string &guid, uint32_t age) {
    std::string relative_path = get_relative_path_str(name, guid, age);
    auto path = std::filesystem::path(downloader::pdb_path_).append(relative_path);
    return path;
}

bool downloader::download_impl(const std::string &relative_path) {
    spdlog::info("download pdb, path: {}", relative_path);
    auto path = std::filesystem::path(downloader::pdb_path_).append(relative_path);
    std::filesystem::create_directories(path.parent_path());

    std::string buf;
    httplib::Client client(downloader::server_split_.first);
    client.set_follow_location(true);
    auto res = client.Get(downloader::server_split_.second + relative_path);
    if (!res || res->status != 200) {
        spdlog::error("failed to download pdb, path: {}", relative_path);
        return false;
    }

    auto tmp_path = path.filename();
    tmp_path.replace_extension(".tmp");
    std::filesystem::remove(tmp_path);
    std::ofstream f(tmp_path, std::ios::binary);
    if (!f.is_open()) {
        spdlog::error("failed to open file, path: {}", tmp_path.string());
        return false;
    }
    f.write(res->body.c_str(), static_cast<std::streamsize>(res->body.size()));
    f.close();

    std::filesystem::rename(tmp_path, path);
    spdlog::info("download pdb success, path: {}", relative_path);
    return true;
}



