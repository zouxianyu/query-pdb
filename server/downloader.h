#ifndef QUERY_PDB_SERVER_DOWNLOADER_H
#define QUERY_PDB_SERVER_DOWNLOADER_H

#include <string>
#include <mutex>
#include <filesystem>

class downloader {

private:
    static std::string pdb_path_;
    static std::string msdl_server_;
    static std::pair<std::string, std::string> server_split_;
public:
    static void set_default_pdb_path(std::string dir_path);
    static void set_default_pdb_server(std::string msdl_server);

public:
    downloader();
    bool download(const std::string &name, const std::string &guid, uint32_t age);
    std::filesystem::path get_path(const std::string &name, const std::string &guid, uint32_t age);

private:
    static std::string get_relative_path_str(const std::string &name, const std::string &guid, uint32_t age);
    bool download_impl(const std::string &relative_path);
};

#endif //QUERY_PDB_SERVER_DOWNLOADER_H
