add_unit_test(ml_plat_crypto_tests platform_crypto_tests.c)
target_link_libraries(ml_plat_crypto_tests PRIVATE moonlight-common-c Threads::Threads)
target_include_directories(ml_plat_crypto_tests PRIVATE ${CMAKE_SOURCE_DIR}/core/moonlight-common-c/src)
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
