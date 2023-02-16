add_compile_definitions(CPPHTTPLIB_OPENSSL_SUPPORT)

add_executable(
        query_pdb_server
        main.cpp
        downloader.cpp
        pdb_parser.cpp
        pdb_helper.cpp
        ExampleMemoryMappedFile.cpp
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

# find openssl library
# on Windows, we use the prebuilt library
# on Linux, we use the library installed by system package manager
if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    message("Windows")
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(OPENSSL_ROOT_DIR ${CMAKE_SOURCE_DIR}/thirdparty/openssl/x64)
        add_custom_command(
                TARGET query_pdb_server
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/thirdparty/openssl/x64/bin/libcrypto-1_1-x64.dll
                ${CMAKE_BINARY_DIR}/server
        )
        add_custom_command(
                TARGET query_pdb_server
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/thirdparty/openssl/x64/bin/libssl-1_1-x64.dll
                ${CMAKE_BINARY_DIR}/server
        )
    else ()
        set(OPENSSL_ROOT_DIR ${CMAKE_SOURCE_DIR}/thirdparty/openssl/x86)
        add_custom_command(
                TARGET query_pdb_server
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/thirdparty/openssl/x86/bin/libcrypto-1_1.dll
                ${CMAKE_BINARY_DIR}/server
        )
        add_custom_command(
                TARGET query_pdb_server
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${CMAKE_SOURCE_DIR}/thirdparty/openssl/x86/bin/libssl-1_1.dll
                ${CMAKE_BINARY_DIR}/server
        )
    endif ()
endif ()
find_package(OpenSSL 1.1.1 REQUIRED)

target_link_libraries(
        query_pdb_server
        PRIVATE
        raw_pdb
        spdlog
        cxxopts
        nlohmann_json
        httplib
        OpenSSL::SSL
        OpenSSL::Crypto
)
