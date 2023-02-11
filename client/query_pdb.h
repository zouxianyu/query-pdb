#ifndef QUERY_PDB_CLIENT_QUERY_PDB_H
#define QUERY_PDB_CLIENT_QUERY_PDB_H

#include <string>
#include <memory>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <utility>
#include <httplib.h>
#include <nlohmann/json.hpp>
#include <windows.h>

#include <iostream>

class qpdb {
public:
    qpdb(const std::string &path, std::string server)
            : pe_(nullptr),
              size_(0),
              server_(std::move(server)),
              info_(),
              valid_(false) {

        if (server_.empty()) {
            return;
        }

        std::ifstream f(path, std::ios::binary);
        if (!f.is_open()) {
            return;
        }

        f.seekg(0, std::ios::end);
        size_ = f.tellg();
        f.seekg(0, std::ios::beg);
        pe_ = std::unique_ptr<std::uint8_t[]>(new std::uint8_t[size_]);
        f.read(reinterpret_cast<char *>(pe_.get()),
               static_cast<std::streamsize>(size_));

        info_ = get_pdb_path_info();
        if (!info_.name.empty() && !info_.guid.empty()) {
            valid_ = true;
        }
    }

    explicit qpdb(const std::string &path)
            : qpdb(path, default_server()) {}

    static std::string &default_server() {
        static std::string default_server_;
        return default_server_;
    }

    void test() {
        std::cout << build_download_path() << std::endl;
    }

private:
    struct pdb_path_info {
        std::string name;
        std::string guid;
        std::uint32_t age{};
    };

    struct raw_debug_info {
        DWORD signature;
        GUID guid;
        DWORD age;
        char pdb_file_name[1];
    };

    std::unique_ptr<std::uint8_t[]> pe_;
    std::size_t size_;
    std::string server_;
    pdb_path_info info_;
    bool valid_;

    static pdb_path_info parse_raw_debug_info(raw_debug_info *raw) {
        pdb_path_info result;
        result.name = raw->pdb_file_name;
        result.age = raw->age;

        std::stringstream ss;
        ss << std::hex << std::setfill('0') << std::uppercase;
        ss << std::setw(8) << raw->guid.Data1;
        ss << std::setw(4) << raw->guid.Data2;
        ss << std::setw(4) << raw->guid.Data3;
        for (unsigned char i: raw->guid.Data4) {
            ss << std::setw(2) << static_cast<std::uint32_t>(i);
        }
        result.guid = ss.str();

        return result;
    }

    pdb_path_info get_pdb_path_info() {
        auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(pe_.get());
        auto nt_header = reinterpret_cast<IMAGE_NT_HEADERS *>(pe_.get() + dos_header->e_lfanew);
        IMAGE_DATA_DIRECTORY *data_directory = nt_header->OptionalHeader.DataDirectory;
        auto debug_directory = reinterpret_cast<IMAGE_DEBUG_DIRECTORY *>(
                pe_.get() + data_directory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress);
        auto raw = reinterpret_cast<raw_debug_info *>(
                pe_.get() + debug_directory->AddressOfRawData);

        return parse_raw_debug_info(raw);
    }

    std::string build_download_path() {
        if (!valid_) {
            return {};
        }

        std::stringstream ss;
        ss << std::hex << std::uppercase;

        ss << server_;
        if (server_.back() != '/') {
            ss << '/';
        }
        ss << info_.name << '/' << info_.guid << info_.age << '/' << info_.name;
        return ss.str();
    }
};

#endif //QUERY_PDB_CLIENT_QUERY_PDB_H
