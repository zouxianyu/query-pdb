#ifndef QUERY_PDB_SERVER_PDB_PARSER_H
#define QUERY_PDB_SERVER_PDB_PARSER_H

#include <string>
#include <memory>
#include <set>
#include <map>
#include <PDB.h>
#include <PDB_RawFile.h>
#include <PDB_DBIStream.h>
#include <PDB_TPIStream.h>
#include "handle_guard.h"

struct field_info {
    int64_t offset;
    int64_t bitfield_offset;

    field_info() : offset(-1), bitfield_offset(0) {}

    std::map<std::string, int64_t> to_map() const {
        return {
                {"offset",          offset},
                {"bitfield_offset", bitfield_offset}
        };
    }
};

class pdb_parser {
public:
    explicit pdb_parser(const std::string &filename);

    std::map<std::string, int64_t> get_symbols(const std::set<std::string> &names) const;

    std::map<std::string, std::map<std::string, field_info>>
    get_struct(const std::map<std::string, std::set<std::string>> &names) const;

    std::map<std::string, std::map<std::string, int64_t>>
    get_enum(const std::map<std::string, std::set<std::string>> &names) const;

private:
    handle_guard file_{};

    static std::map<std::string, int64_t> get_symbols_impl(
            const PDB::RawFile &raw_file,
            const PDB::DBIStream &dbi_stream,
            const std::set<std::string> &names
    );

    static std::map<std::string, std::map<std::string, field_info>>
    get_struct_impl(
            const PDB::TPIStream &tpi_stream,
            const std::map<std::string, std::set<std::string>> &names
    );

    static std::map<std::string, field_info>
    get_struct_single(
            const PDB::TPIStream &tpi_stream,
            const PDB::CodeView::TPI::Record *record,
            const std::set<std::string> &names
    );

    static std::map<std::string, std::map<std::string, int64_t>>
    get_enum_impl(
            const PDB::TPIStream &tpi_stream,
            const std::map<std::string, std::set<std::string>> &names
    );

    static std::map<std::string, int64_t>
    get_enum_single(
            const PDB::CodeView::TPI::Record *record,
            uint8_t underlying_type_size,
            const std::set<std::string> &names
    );
};

#endif //QUERY_PDB_SERVER_PDB_PARSER_H
