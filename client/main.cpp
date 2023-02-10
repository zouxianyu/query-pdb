#include "query_pdb.h"

int main() {
    qpdb::default_server() = "http://msdl.microsoft.com/download/symbols/";
    qpdb pdb(R"(C:\Windows\System32\ntoskrnl.exe)");
    pdb.test();
    return 0;
}
