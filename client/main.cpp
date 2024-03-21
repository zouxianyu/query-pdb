#include <iostream>
#include "query_pdb.h"

int main() {
    qpdb::set_default_server("http://www.zzzou.xyz:9025");

    try {
        qpdb pdb(R"(C:\Windows\System32\ntoskrnl.exe)");

        // output hex value
        std::cout << std::hex;

        ////////////////////////////////////////////////////////////////////////
        // query global offset
        {
            // method 1
            auto offset = pdb.get_symbol("KdpStub");
            std::cout << offset << std::endl;
        }

        {
            // method 2
            auto offsets = pdb.get_symbol(std::set<std::string>{
                    "KdpStub",
                    "MmAccessFault"
            });

            std::cout << offsets["KdpStub"] << std::endl;
            std::cout << offsets["MmAccessFault"] << std::endl;
        }

        ////////////////////////////////////////////////////////////////////////
        // query struct field offset
        {
            // method 1
            auto field = pdb.get_struct("_KPROCESS", "DirectoryTableBase");

            std::cout << field.offset << ", " << field.bitfield_offset << std::endl;
        }

        {
            // method 2
            auto fields = pdb.get_struct(
                    "_KPROCESS", std::set<std::string>{
                            "DirectoryTableBase",
                            "DisableQuantum"
                    }
            );

            std::cout << fields["DirectoryTableBase"].offset <<
                      fields["DirectoryTableBase"].bitfield_offset << std::endl;

            std::cout << fields["DisableQuantum"].offset <<
                      fields["DisableQuantum"].bitfield_offset << std::endl;
        }

        {
            // method 3
            auto structs = pdb.get_struct({
                    {"_KPROCESS", {
                            "DirectoryTableBase",
                            "DisableQuantum"
                    }},
                    {"_KTHREAD", {
                            "Teb"
                    }}
            });

            std::cout << structs["_KTHREAD"]["Teb"].offset <<
                      structs["_KTHREAD"]["Teb"].bitfield_offset << std::endl;
        }

        ////////////////////////////////////////////////////////////////////////
        // query enum value

        {
            // method 1
            auto value = pdb.get_enum("_POOL_TYPE", "PagedPool");

            std::cout << value << std::endl;
        }

        {
            // method 2
            auto values = pdb.get_enum(
                    "_POOL_TYPE", std::set<std::string>{
                            "PagedPool",
                            "NonPagedPool"
                    }
            );

            std::cout << values["PagedPool"] << std::endl;
            std::cout << values["NonPagedPool"] << std::endl;
        }

        {
            // method 3
            auto enums = pdb.get_enum({
                    {"_POOL_TYPE",        {
                            "PagedPool",
                            "NonPagedPool"
                    }},
                    {"_EX_POOL_PRIORITY", {
                            "NormalPoolPriority"
                    }}
            });

            std::cout << enums["_POOL_TYPE"]["PagedPool"] << std::endl;
        }

    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
