add_executable(
        query_pdb_client
        main.cpp
)

set_target_properties(
        query_pdb_client
        PROPERTIES
        CXX_STANDARD 11
)

target_include_directories(
        query_pdb_client
        PRIVATE
        ${CMAKE_SOURCE_DIR}/lib
)

target_link_libraries(
        query_pdb_client
        PRIVATE
        ws2_32.lib
)
