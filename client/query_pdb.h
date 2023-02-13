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

class qpdb {
public:
    qpdb(const std::string &path, std::string server)
            : pe_(),
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

        std::ostringstream ss;
        ss << f.rdbuf();
        pe_ = ss.str();

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

    static void set_default_server(const std::string &server) {
        default_server() = server;
    }

    std::map<std::string, int64_t> get_symbol(const std::set<std::string> &names) const {
        if (!valid_) {
            throw std::runtime_error("invalid file, cannot get pdb info");
        }

        nlohmann::json j;
        j["name"] = info_.name;
        j["guid"] = info_.guid;
        j["age"] = info_.age;
        j["query"] = names;

        httplib::Client client(server_);
        auto res = client.Post("/symbol", j.dump(), "application/json");
        if (!res || res->status != 200) {
            throw std::runtime_error("request failed");
        }

        return nlohmann::json::parse(res->body).get<std::map<std::string, int64_t>>();
    }

    int64_t get_symbol(const std::string &name) const {
        return get_symbol(std::set<std::string>{name}).at(name);
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

    std::string pe_;
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

    static uint32_t rva_to_offset(char *pe, uint32_t rva) {
        auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(pe);
        auto nt_header = reinterpret_cast<IMAGE_NT_HEADERS *>(pe + dos_header->e_lfanew);
        auto section_header = IMAGE_FIRST_SECTION(nt_header);
        for (int i = 0; i < nt_header->FileHeader.NumberOfSections; i++) {
            if (rva >= section_header[i].VirtualAddress &&
                rva < section_header[i].VirtualAddress + section_header[i].Misc.VirtualSize) {
                return rva - section_header[i].VirtualAddress + section_header[i].PointerToRawData;
            }
        }
        return 0;
    }

    pdb_path_info get_pdb_path_info() {
        auto p = const_cast<char *>(pe_.c_str());
        auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER *>(p);
        auto nt_header = reinterpret_cast<IMAGE_NT_HEADERS *>(p + dos_header->e_lfanew);

        IMAGE_DATA_DIRECTORY *data_directory;
        switch (nt_header->FileHeader.Machine) {
            case IMAGE_FILE_MACHINE_I386:
                data_directory = reinterpret_cast<IMAGE_NT_HEADERS32 *>(nt_header)->
                        OptionalHeader.DataDirectory;
                break;
            case IMAGE_FILE_MACHINE_AMD64:
                data_directory = reinterpret_cast<IMAGE_NT_HEADERS64 *>(nt_header)->
                        OptionalHeader.DataDirectory;
                break;
            default:
                throw std::runtime_error("unsupported machine type");
        }
        auto debug_directory = reinterpret_cast<IMAGE_DEBUG_DIRECTORY *>(
                p + rva_to_offset(p, data_directory[IMAGE_DIRECTORY_ENTRY_DEBUG].VirtualAddress));
        auto raw = reinterpret_cast<raw_debug_info *>(p + debug_directory->PointerToRawData);

        return parse_raw_debug_info(raw);
    }

};

#endif //QUERY_PDB_CLIENT_QUERY_PDB_H
