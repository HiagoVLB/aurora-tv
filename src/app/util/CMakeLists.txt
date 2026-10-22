target_sources(moonlight-lib PRIVATE
        path.c
        img_loader.c
        nullable.c
        font.c
        log_overlay.c
        file_log.c
        diagnostic_reset.c
        crash_log.c)

if (TARGET_WEBOS)
    find_path(CRASH_UNWIND_INCLUDE_DIR libunwind.h REQUIRED)
    target_include_directories(moonlight-lib PRIVATE ${CRASH_UNWIND_INCLUDE_DIR})
    target_compile_definitions(moonlight-lib PRIVATE CRASH_LOG_UNWIND=1)
    target_link_libraries(moonlight-lib PRIVATE ${CMAKE_DL_LIBS})
    # Keep ARM unwind metadata in our code without changing streaming policy.
    foreach (diagnostic_target moonlight moonlight-lib moonlight-common-c enet)
        target_compile_options(${diagnostic_target} PRIVATE -funwind-tables)
    endforeach ()
endif ()
