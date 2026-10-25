add_unit_test(ml_plat_crypto_tests platform_crypto_tests.c)

# Guard the nanors work buffer, including recovery of every data shard.
add_executable(test_nanors test_nanors.c ../../core/moonlight-common-c/nanors/rs.c)
target_include_directories(test_nanors PRIVATE ../../core/moonlight-common-c/nanors
    ../../core/moonlight-common-c/nanors/deps ../../core/moonlight-common-c/nanors/deps/obl)
target_compile_definitions(test_nanors PRIVATE OBLAS_TINY)
target_compile_options(test_nanors PRIVATE -UNDEBUG)
add_test(NAME test_nanors COMMAND test_nanors)
target_link_libraries(ml_plat_crypto_tests PRIVATE moonlight-common-c Threads::Threads)
target_include_directories(ml_plat_crypto_tests PRIVATE ${CMAKE_SOURCE_DIR}/core/moonlight-common-c/src)

add_executable(test_http_lifecycle test_http_lifecycle.c ../../core/libgamestream/src/http.c)
target_include_directories(test_http_lifecycle PRIVATE
    ../../core/libgamestream/libgamestream ../../core/libgamestream/src
    ../../third_party/commons/util/logging ${CURL_INCLUDE_DIRS})
target_compile_definitions(test_http_lifecycle PRIVATE CURL_DISABLE_TYPECHECK _DEFAULT_SOURCE)
target_link_libraries(test_http_lifecycle PRIVATE Threads::Threads)
add_test(NAME test_http_lifecycle COMMAND test_http_lifecycle)
add_test(NAME test_http_init_failure COMMAND test_http_lifecycle fail-init)
# Isolate HTTP and random generation so real GameStream/XML handling can be
# exercised without pairing or contacting a host. Discard unrelated client APIs.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(test_vibeshine_protocol test_vibeshine_protocol.c
        ../../core/libgamestream/src/client.c
        ../../core/libgamestream/src/xml.c
        ../../core/libgamestream/src/set_error.c)
    target_include_directories(test_vibeshine_protocol PRIVATE
        ../../core/libgamestream/libgamestream ../../core/libgamestream/src
        ../../core/moonlight-common-c/src ../../third_party/commons/util/logging
        ../../third_party/commons/util/uuidstr ${MBEDTLS_INCLUDE_DIRS} ${EXPAT_INCLUDE_DIRS})
    target_compile_options(test_vibeshine_protocol PRIVATE -UNDEBUG -ffunction-sections -fdata-sections)
    target_link_options(test_vibeshine_protocol PRIVATE -Wl,--gc-sections)
    target_link_libraries(test_vibeshine_protocol PRIVATE ${EXPAT_LIBRARIES})
    if (TARGET ext_mbedtls)
        add_dependencies(test_vibeshine_protocol ext_mbedtls)
    endif ()
    add_test(NAME test_vibeshine_protocol COMMAND test_vibeshine_protocol)
endif ()
