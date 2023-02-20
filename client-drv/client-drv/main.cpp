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
        kqpdb::set_default_server("http://39.105.177.215:9025/");
        kqpdb pdb("\\SystemRoot\\System32\\ntoskrnl.exe");

        ////////////////////////////////////////////////////////////////////////
        // query global offset
        {
            // method 1
            auto offset = pdb.get_symbol("KdpStub");
            DbgPrint("KdpStub: %x\n", offset);
        }

        {
            // method 2
            auto offsets = pdb.get_symbol(std::set<std::string>{
                    "KdpStub",
                    "MmAccessFault"
            });

            DbgPrint("KdpStub: %x\n", offsets["KdpStub"]);
            DbgPrint("MmAccessFault: %x\n", offsets["MmAccessFault"]);
        }

        ////////////////////////////////////////////////////////////////////////
        // query struct field offset
        {
            // method 1
            auto field = pdb.get_struct("_KPROCESS", "DirectoryTableBase");

            DbgPrint("DirectoryTableBase: %x, %x\n", field.offset, field.bitfield_offset);
        }

        {
            // method 2
            auto fields = pdb.get_struct(
                    "_KPROCESS", std::set<std::string>{
                            "DirectoryTableBase",
                            "DisableQuantum"
                    }
            );

            DbgPrint("DirectoryTableBase: %x, %x\n", fields["DirectoryTableBase"].offset,
                     fields["DirectoryTableBase"].bitfield_offset);

            DbgPrint("DisableQuantum: %x, %x\n", fields["DisableQuantum"].offset,
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

            DbgPrint("DirectoryTableBase: %x, %x\n",
                     structs["_KPROCESS"]["DirectoryTableBase"].offset,
                     structs["_KPROCESS"]["DirectoryTableBase"].bitfield_offset);
        }

        ////////////////////////////////////////////////////////////////////////
        // query enum value

        {
            // method 1
            auto value = pdb.get_enum("_POOL_TYPE", "PagedPool");

            DbgPrint("PagedPool: %x\n", value);
        }

        {
            // method 2
            auto values = pdb.get_enum(
                    "_POOL_TYPE", std::set<std::string>{
                            "PagedPool",
                            "NonPagedPool"
                    }
            );

            DbgPrint("PagedPool: %x\n", values["PagedPool"]);
            DbgPrint("NonPagedPool: %x\n", values["NonPagedPool"]);
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

            DbgPrint("PagedPool: %x\n", enums["_POOL_TYPE"]["PagedPool"]);
        }

    } catch (std::exception &e) {
        DbgPrint("[DriverMain] exception: %s\n", e.what());
    } catch (...) {
        DbgPrint("[DriverMain] exception\n");
    }

    DbgPrint("[DriverMain] start successfully\n");
    return STATUS_SUCCESS;
}