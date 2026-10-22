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

add_executable(test_diagnostic_reset test_diagnostic_reset.c ../../src/app/util/diagnostic_reset.c)
target_include_directories(test_diagnostic_reset PRIVATE ../../src/app)
add_test(NAME test_diagnostic_reset COMMAND test_diagnostic_reset)
