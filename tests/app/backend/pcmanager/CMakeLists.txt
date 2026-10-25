add_unit_test(test_known_hosts test_known_hosts.c)

add_executable(test_quit_app test_quit_app.c)
target_compile_options(test_quit_app PRIVATE -UNDEBUG)
target_link_libraries(test_quit_app PRIVATE moonlight-lib)
add_test(NAME test_quit_app COMMAND test_quit_app)

add_subdirectory(discovery)
