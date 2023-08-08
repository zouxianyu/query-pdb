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
    struct field_info {
        int64_t offset;
        int64_t bitfield_offset;

        static field_info from_map(const std::map<std::string, int64_t> &m) {
            field_info info{};
            info.offset = m.at("offset");
            info.bitfield_offset = m.at("bitfield_offset");
            return info;
        }
    };

    explicit qpdb(const std::string &path, std::string server = default_server(),
                  uint32_t timeout = 180)
            : pe_(),
              server_(std::move(server)),
              info_(),
              valid_(false),
              timeout_(timeout) {

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
        client.set_read_timeout(timeout_);
        auto res = client.Post("/symbol", j.dump(), "application/json");
        if (!res || res->status != 200) {
            throw std::runtime_error("request failed");
        }

        return nlohmann::json::parse(res->body).get<std::map<std::string, int64_t>>();
    }

    int64_t get_symbol(const std::string &name) const {
        return get_symbol(std::set<std::string>{name}).at(name);
    }

    std::map<std::string, std::map<std::string, field_info>>
    get_struct(const std::map<std::string, std::set<std::string>> &names) const {
        if (!valid_) {
            throw std::runtime_error("invalid file, cannot get pdb info");
        }

        nlohmann::json j;
        j["name"] = info_.name;
        j["guid"] = info_.guid;
        j["age"] = info_.age;
        j["query"] = names;

        httplib::Client client(server_);
        client.set_read_timeout(timeout_);
        auto res = client.Post("/struct", j.dump(), "application/json");
        if (!res || res->status != 200) {
            throw std::runtime_error("request failed");
        }

        auto result = nlohmann::json::parse(res->body)
                .get<std::map<std::string, std::map<std::string, std::map<std::string, int64_t>>>>();

        std::map<std::string, std::map<std::string, field_info>> field_info_map;
        for (const auto &struct_info: result) {
            field_info_map[struct_info.first] = {};
            for (const auto &field_info: struct_info.second) {
                field_info_map[struct_info.first][field_info.first] =
                        field_info::from_map(field_info.second);
            }
        }
        return field_info_map;
    }

    std::map<std::string, field_info>
    get_struct(const std::string &name, const std::set<std::string> &fields) const {
        return get_struct(std::map<std::string, std::set<std::string>>{{name, fields}})
                .at(name);
    }

    field_info get_struct(const std::string &name, const std::string &field) const {
        return get_struct(name, std::set<std::string>{field}).at(field);
    }

    std::map<std::string, std::map<std::string, int64_t>>
    get_enum(const std::map<std::string, std::set<std::string>> &names) const {
        if (!valid_) {
            throw std::runtime_error("invalid file, cannot get pdb info");
        }

        nlohmann::json j;
        j["name"] = info_.name;
        j["guid"] = info_.guid;
        j["age"] = info_.age;
        j["query"] = names;

        httplib::Client client(server_);
        client.set_read_timeout(timeout_);
        auto res = client.Post("/enum", j.dump(), "application/json");
        if (!res || res->status != 200) {
            throw std::runtime_error("request failed");
        }

        return nlohmann::json::parse(res->body)
                .get<std::map<std::string, std::map<std::string, int64_t>>>();
    }

    std::map<std::string, int64_t>
    get_enum(const std::string &name, const std::set<std::string> &keys) const {
        return get_enum(std::map<std::string, std::set<std::string>>{{name, keys}})
                .at(name);
    }

    int64_t get_enum(const std::string &name, const std::string &key) const {
        return get_enum(name, std::set<std::string>{key}).at(key);
    }

private:
    struct pdb_path_info {
        std::string name;
        std::string guid;
        std::uint32_t age{};
    };

    typedef struct _CV_INFO_PDB70 {
        DWORD      CvSignature;
        GUID       Signature;
        DWORD      Age;
        char       PdbFileName[1];
    } CV_INFO_PDB70;

    typedef struct _CV_INFO_PDB20 {
        DWORD      CvSignature;
        DWORD      Offset;
        DWORD      Signature;
        DWORD      Age;
        char       PdbFileName[1];
    } CV_INFO_PDB20;

    std::string pe_;
    std::string server_;
    pdb_path_info info_;
    bool valid_;
    uint32_t timeout_;

    static pdb_path_info parse_raw_debug_info(char *raw) {
        pdb_path_info result;
        std::stringstream ss;
        if (strncmp((const char*)raw, "RSDS", 4) == 0) {
            //pdb 7.0
            auto pdb7 = reinterpret_cast<CV_INFO_PDB70*>(raw);
            GUID uuid = pdb7->Signature;
            result.name = pdb7->PdbFileName;
            result.age = pdb7->Age;
            ss << std::hex << std::setfill('0') << std::uppercase;
            ss << std::setw(8) << uuid.Data1;
            ss << std::setw(4) << uuid.Data2;
            ss << std::setw(4) << uuid.Data3;
            for (uint8_t i : uuid.Data4) {
                ss << std::setw(2) << static_cast<std::uint32_t>(i);
            }
            result.guid = ss.str();
        } else if (strncmp((const char*)raw, "NB10", 4) == 0) {
            //pdb 2.0
            auto pdb2 = reinterpret_cast<_CV_INFO_PDB20*>(raw);
            result.name = pdb2->PdbFileName;
            result.age = pdb2->Age;
            ss << std::hex << std::setfill('0') << std::uppercase;
            ss << std::setw(8) << pdb2->Signature;
            result.guid = ss.str();
        }
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

        if (debug_directory->Type != IMAGE_DEBUG_TYPE_CODEVIEW || debug_directory->SizeOfData == 0) {
            return pdb_path_info();
        }
        auto raw = p + debug_directory->PointerToRawData;

        return parse_raw_debug_info(raw);
    }

};

#endif //QUERY_PDB_CLIENT_QUERY_PDB_H
