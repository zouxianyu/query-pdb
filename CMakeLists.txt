cmake_minimum_required(VERSION 3.0)
project(query_pdb)

add_subdirectory(thirdparty/cpp-httplib)
add_subdirectory(thirdparty/cxxopts)
add_subdirectory(thirdparty/nlohmann_json)
add_subdirectory(thirdparty/raw_pdb)
add_subdirectory(thirdparty/spdlog)

add_subdirectory(client)
add_subdirectory(server)
