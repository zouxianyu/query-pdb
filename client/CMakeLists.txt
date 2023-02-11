add_executable(
        query_pdb_client
        main.cpp
)

set_target_properties(
        query_pdb_client
        PROPERTIES
        CXX_STANDARD 11
)

target_link_libraries(
        query_pdb_client
        PRIVATE
        nlohmann_json
        httplib
)
