#include <iostream>
#include "query_pdb.h"

int main() {
    qpdb::set_default_server("http://localhost:8080/");

    try {
        qpdb pdb(R"(D:\ntoskrnl.exe)");
        int64_t offset = pdb.get_symbol("KdpStub");
        std::cout << "KdpStub: " << std::hex << offset << std::endl;
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
