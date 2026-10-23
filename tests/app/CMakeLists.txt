add_subdirectory(e2e)

add_unit_test(test_settings test_settings.c)

add_subdirectory(backend)
add_subdirectory(ui)
# Compile the writer directly with a small cap to exercise many rollovers.
add_executable(test_file_log test_file_log.c ../../src/app/util/file_log.c)
target_include_directories(test_file_log PRIVATE ../../src/app ../../third_party/commons/util/logging)
target_compile_definitions(test_file_log PRIVATE FILE_LOG_MAX_BYTES=4096 _POSIX_C_SOURCE=200809L)
target_link_libraries(test_file_log PRIVATE Threads::Threads)
add_test(NAME test_file_log COMMAND test_file_log)

add_executable(test_crash_log test_crash_log.c ../../src/app/util/crash_log.c)
target_include_directories(test_crash_log PRIVATE ../../src/app)
target_link_libraries(test_crash_log PRIVATE Threads::Threads)
add_test(NAME test_crash_log COMMAND test_crash_log)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    pkg_check_modules(TEST_CRASH_UNWIND QUIET libunwind)
    if (TEST_CRASH_UNWIND_FOUND)
        foreach (crash_test test_crash_log_unwind test_crash_log_unwind_missing test_crash_log_unwind_fault)
            add_executable(${crash_test} test_crash_log.c ../../src/app/util/crash_log.c)
            target_include_directories(${crash_test} PRIVATE ../../src/app ${TEST_CRASH_UNWIND_INCLUDE_DIRS})
            target_compile_definitions(${crash_test} PRIVATE CRASH_LOG_UNWIND=1)
            target_compile_options(${crash_test} PRIVATE -funwind-tables -fno-optimize-sibling-calls)
            target_link_libraries(${crash_test} PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
            add_test(NAME ${crash_test} COMMAND ${crash_test})
        endforeach ()
        target_compile_definitions(test_crash_log_unwind_missing PRIVATE CRASH_LOG_TEST_NO_UNWIND=1)
        target_link_options(test_crash_log_unwind_missing PRIVATE -Wl,--wrap=dlopen)
        target_compile_definitions(test_crash_log_unwind_fault PRIVATE CRASH_LOG_TEST_FAULTY_UNWIND=1)
        target_link_options(test_crash_log_unwind_fault PRIVATE -Wl,--wrap=dlsym)
    endif ()
endif ()

add_executable(test_diagnostic_reset test_diagnostic_reset.c ../../src/app/util/diagnostic_reset.c)
target_include_directories(test_diagnostic_reset PRIVATE ../../src/app)
add_test(NAME test_diagnostic_reset COMMAND test_diagnostic_reset)

add_executable(test_decode_unit test_decode_unit.c ../../src/app/stream/video/decode_unit.c)
target_include_directories(test_decode_unit PRIVATE ../../src/app ../../core/moonlight-common-c/src)
add_test(NAME test_decode_unit COMMAND test_decode_unit)

# Exercise the actual webOS driver with simulated firmware entry points on Linux.
if (CMAKE_SYSTEM_NAME STREQUAL "Linux" AND NOT TARGET_WEBOS)
    set(TEST_NDL_DIR ${PROJECT_SOURCE_DIR}/third_party/ss4s/modules/webos/ndl/webos5)
    add_executable(test_ndl_serialization test_ndl_serialization.c
        ${TEST_NDL_DIR}/ndl_module.c ${TEST_NDL_DIR}/ndl_player.c
        ${TEST_NDL_DIR}/ndl_video.c ${TEST_NDL_DIR}/ndl_audio.c
        ${TEST_NDL_DIR}/opus_empty.c ${TEST_NDL_DIR}/opus_fix.c
        ${TEST_NDL_DIR}/../../common/aurora_frame_diag.c)
    target_include_directories(test_ndl_serialization PRIVATE ${TEST_NDL_DIR}
        ${PROJECT_SOURCE_DIR}/third_party/ss4s/mocks/include/libndl-media
        ${PROJECT_SOURCE_DIR}/third_party/ss4s/modules/webos/utils ${OPUS_INCLUDE_DIRS})
    target_compile_definitions(test_ndl_serialization PRIVATE _GNU_SOURCE)
    target_link_libraries(test_ndl_serialization PRIVATE ss4s-module-api Threads::Threads
        ${CMAKE_DL_LIBS} ${OPUS_LIBRARIES} m)
    add_test(NAME test_ndl_serialization COMMAND test_ndl_serialization)
    set_tests_properties(test_ndl_serialization PROPERTIES TIMEOUT 75)
endif ()
