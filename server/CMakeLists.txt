add_executable(
        query_pdb_server
        main.cpp
        downloader.cpp
)

set_target_properties(
        query_pdb_server
        PROPERTIES
        CXX_STANDARD 17
)

target_include_directories(
        query_pdb_server
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(
        query_pdb_server
        PRIVATE
        raw_pdb
        spdlog
        cxxopts
        nlohmann_json
        httplib
)
