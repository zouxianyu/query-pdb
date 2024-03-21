#include <ucxxrt.h>
#include "kquery_pdb.h"

EXTERN_C
VOID DriverUnload(PDRIVER_OBJECT pDriverObj) {
    UNREFERENCED_PARAMETER(pDriverObj);

    DbgPrint("[DriverUnload]\n");
}

EXTERN_C
NTSTATUS DriverMain(PDRIVER_OBJECT pDriverObj, PUNICODE_STRING pRegistryString) {
    UNREFERENCED_PARAMETER(pRegistryString);

    pDriverObj->DriverUnload = DriverUnload;

    try {
        kqpdb::set_default_server("http://www.zzzou.xyz:9025");
        kqpdb pdb("\\SystemRoot\\System32\\ntoskrnl.exe");

        ////////////////////////////////////////////////////////////////////////
        // query global offset
        {
            // method 1
            auto offset = pdb.get_symbol("KdpStub");
            DbgPrint("KdpStub: %llx\n", offset);
        }

        {
            // method 2
            auto offsets = pdb.get_symbol(std::set<std::string>{
                    "KdpStub",
                    "MmAccessFault"
            });

            DbgPrint("KdpStub: %llx\n", offsets["KdpStub"]);
            DbgPrint("MmAccessFault: %llx\n", offsets["MmAccessFault"]);
        }

        ////////////////////////////////////////////////////////////////////////
        // query struct field offset
        {
            // method 1
            auto field = pdb.get_struct("_KPROCESS", "DirectoryTableBase");

            DbgPrint("DirectoryTableBase: %llx, %llx\n", field.offset, field.bitfield_offset);
        }

        {
            // method 2
            auto fields = pdb.get_struct(
                    "_KPROCESS", std::set<std::string>{
                            "DirectoryTableBase",
                            "DisableQuantum"
                    }
            );

            DbgPrint("DirectoryTableBase: %llx, %llx\n", fields["DirectoryTableBase"].offset,
                     fields["DirectoryTableBase"].bitfield_offset);

            DbgPrint("DisableQuantum: %llx, %llx\n", fields["DisableQuantum"].offset,
                     fields["DisableQuantum"].bitfield_offset);
        }

        {
            // method 3
            auto structs = pdb.get_struct({
                {"_KPROCESS", {
                        "DirectoryTableBase",
                        "DisableQuantum"
                }},
                {"_KTHREAD",  {
                        "Teb"
                }}
            });

            DbgPrint("DirectoryTableBase: %llx, %llx\n",
                     structs["_KPROCESS"]["DirectoryTableBase"].offset,
                     structs["_KPROCESS"]["DirectoryTableBase"].bitfield_offset);
        }

        ////////////////////////////////////////////////////////////////////////
        // query enum value

        {
            // method 1
            auto value = pdb.get_enum("_POOL_TYPE", "PagedPool");

            DbgPrint("PagedPool: %llx\n", value);
        }

        {
            // method 2
            auto values = pdb.get_enum(
                    "_POOL_TYPE", std::set<std::string>{
                            "PagedPool",
                            "NonPagedPool"
                    }
            );

            DbgPrint("PagedPool: %llx\n", values["PagedPool"]);
            DbgPrint("NonPagedPool: %llx\n", values["NonPagedPool"]);
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

            DbgPrint("PagedPool: %llx\n", enums["_POOL_TYPE"]["PagedPool"]);
        }

    } catch (std::exception &e) {
        DbgPrint("[DriverMain] exception: %s\n", e.what());
    } catch (...) {
        DbgPrint("[DriverMain] exception\n");
    }

    DbgPrint("[DriverMain] start successfully\n");
    return STATUS_SUCCESS;
}