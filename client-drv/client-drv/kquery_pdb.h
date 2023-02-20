#ifndef QPDB_DRIVER_KQUERY_PDB_H
#define QPDB_DRIVER_KQUERY_PDB_H

#include <string>
#include <set>
#include <map>
#include <windef.h>
#include <ntimage.h>

#define JSON_NOEXCEPTION
#define JSON_NO_IO
#define JSON_ASSERT(x) (void)0

#include "json.hpp"

#include "khttp.h"

class kqpdb {
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

    explicit kqpdb(const std::string &path, std::string server = default_server())
            : pe_(),
              server_(std::move(server)),
              info_(),
              valid_(false) {

        if (server_.empty()) {
            return;
        }

        kfile f(path);
        if (!f.valid()) {
            KdPrint(("kqpdb: open file failed: %s\n"));
            return;
        }
        pe_ = f.read_all();
        if (pe_.empty()) {
            KdPrint(("kqpdb: read file failed: %s\n"));
            return;
        }

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

        khttp client(server_);
        auto res = client.post("/symbol", j.dump());
        if (res.statusCode != 200) {
            throw std::runtime_error("request failed");
        }

        return nlohmann::json::parse(to_string(res.content))
                .get<std::map<std::string, int64_t>>();
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

        khttp client(server_);
        auto res = client.post("/struct", j.dump());
        if (res.statusCode != 200) {
            throw std::runtime_error("request failed");
        }

        auto result = nlohmann::json::parse(to_string(res.content))
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

        khttp client(server_);
        auto res = client.post("/enum", j.dump());
        if (res.statusCode != 200) {
            throw std::runtime_error("request failed");
        }

        return nlohmann::json::parse(to_string(res.content))
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
    class kfile {
    public:
        explicit kfile(const std::string &path)
                : file_(nullptr) {

            UNICODE_STRING ustr;
            const std::wstring wpath(path.begin(), path.end());
            RtlInitUnicodeString(&ustr, wpath.c_str());

            OBJECT_ATTRIBUTES ObjectAttributes;
            InitializeObjectAttributes(
                    &ObjectAttributes,
                    &ustr,
                    OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                    nullptr,
                    nullptr
            );

            IO_STATUS_BLOCK IoStatusBlock;

            NTSTATUS status = ZwOpenFile(
                    &file_,
                    FILE_GENERIC_READ,
                    &ObjectAttributes,
                    &IoStatusBlock,
                    FILE_SHARE_READ,
                    FILE_NON_DIRECTORY_FILE
            );

            if (status != STATUS_SUCCESS) {
                KdPrint(("[kfile] ZwOpenFile failed: %x\n", status));
            }
        }

        bool valid() const {
            return file_ != nullptr;
        }

        std::string read_all() {
            if (!file_) {
                return {};
            }

            FILE_STANDARD_INFORMATION info;
            IO_STATUS_BLOCK IoStatusBlock;
            NTSTATUS status = ZwQueryInformationFile(
                    file_,
                    &IoStatusBlock,
                    &info,
                    sizeof(info),
                    FileStandardInformation
            );

            if (status != STATUS_SUCCESS) {
                KdPrint(("[kfile] ZwQueryInformationFile failed: %x\n", status));
                return {};
            }

            if (info.EndOfFile.QuadPart <= 0) {
                return {};
            }


            std::string content;
            content.resize(info.EndOfFile.QuadPart);
            for (auto &c: content) {
                c = 0;
            }

            LARGE_INTEGER byte_offset{};

            status = ZwReadFile(
                    file_,
                    nullptr,
                    nullptr,
                    nullptr,
                    &IoStatusBlock,
                    &content[0],
                    info.EndOfFile.QuadPart,
                    &byte_offset,
                    nullptr
            );

            if (status == STATUS_PENDING) {
                ZwWaitForSingleObject(file_, false, nullptr);
                status = IoStatusBlock.Status;
            }

            if (status != STATUS_SUCCESS) {
                KdPrint(("[kfile] ZwReadFile failed: %x\n", status));
                return {};
            }

            return content;
        }

        ~kfile() {
            if (file_) {
                ZwClose(file_);
            }
        }

    private:
        HANDLE file_;
    };

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

    static std::string to_string(const std::vector<char> &vec) {
        return {vec.begin(), vec.end()};
    }

    static pdb_path_info parse_raw_debug_info(raw_debug_info *raw) {
        pdb_path_info result;
        result.name = raw->pdb_file_name;
        result.age = raw->age;

        char guid[33];
        sprintf(guid, "%08lX%04X%04X%02X%02X%02X%02X%02X%02X%02X%02X",
                raw->guid.Data1,
                raw->guid.Data2,
                raw->guid.Data3,
                raw->guid.Data4[0],
                raw->guid.Data4[1],
                raw->guid.Data4[2],
                raw->guid.Data4[3],
                raw->guid.Data4[4],
                raw->guid.Data4[5],
                raw->guid.Data4[6],
                raw->guid.Data4[7]);
        result.guid = guid;

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

#endif //QPDB_DRIVER_KQUERY_PDB_H
