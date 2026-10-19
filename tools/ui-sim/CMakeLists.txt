# A host-only executable; no test command channel is compiled into Aurora/IPKs.
add_executable(aurora-ui-sim main.c)
target_link_libraries(aurora-ui-sim PRIVATE moonlight-lib)
add_dependencies(aurora-ui-sim moonlight-i18n)
