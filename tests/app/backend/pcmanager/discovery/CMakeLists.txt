add_unit_test(test_throttle test_throttle.c)
# No actual mDNS: force early failure and shutdown during an in-flight callback.
add_executable(test_discovery_lifecycle test_discovery_lifecycle.c
    ../../../../../src/app/backend/pcmanager/discovery/discovery.c)
target_include_directories(test_discovery_lifecycle PRIVATE
    ../../../../../src/app ../../../../../third_party/commons/net/sockaddr
    ../../../../../third_party/commons/util/logging ${SDL2_INCLUDE_DIRS})
target_compile_options(test_discovery_lifecycle PRIVATE -UNDEBUG)
target_link_libraries(test_discovery_lifecycle PRIVATE ${SDL2_LIBRARIES})
add_test(NAME test_discovery_lifecycle COMMAND test_discovery_lifecycle)
set_tests_properties(test_discovery_lifecycle PROPERTIES TIMEOUT 10)
