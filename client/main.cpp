#include "query_pdb.hpp"

int main() {
    qpdb pdb(R"(C:\Windows\System32\ntoskrnl.exe)");
    pdb.test();
    return 0;
}
