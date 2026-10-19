find_program(MSGFMT_COMMAND msgfmt)
find_program(XGETTEXT_COMMAND xgettext)
find_program(MSGMERGE_COMMAND msgmerge)

if (NOT MSGFMT_COMMAND OR NOT XGETTEXT_COMMAND OR NOT MSGMERGE_COMMAND)
    message(FATAL_ERROR "msgfmt not found. Please install gettext.")
endif ()
set(MOFILES)
set(_I18N_GETTEXT_LOCALES ${I18N_LOCALES})
if (BUILD_UI_SIM)
    # Exercise existing inactive catalogs without adding them to the TV selector/package.
    file(GLOB _I18N_SIM_CATALOGS LIST_DIRECTORIES FALSE "${CMAKE_SOURCE_DIR}/src/i18n/*/messages.po")
    foreach (_CATALOG ${_I18N_SIM_CATALOGS})
        get_filename_component(_CATALOG_DIR "${_CATALOG}" DIRECTORY)
        get_filename_component(_CATALOG_LANG "${_CATALOG_DIR}" NAME)
        list(APPEND _I18N_GETTEXT_LOCALES "${_CATALOG_LANG}")
    endforeach ()
    list(REMOVE_DUPLICATES _I18N_GETTEXT_LOCALES)
endif ()
foreach (LANG ${_I18N_GETTEXT_LOCALES})
    string(REPLACE "-" "_" _MOLANG ${LANG})
    set(_POFILE ${CMAKE_SOURCE_DIR}/src/i18n/${LANG}/messages.po)
    set(_MODIR ${CMAKE_BINARY_DIR}/mo/${_MOLANG}/LC_MESSAGES)
    set(_MOFILE ${_MODIR}/moonlight-tv.mo)
    add_custom_command(OUTPUT ${_MOFILE} DEPENDS src/i18n/${LANG}/messages.po
            COMMAND ${CMAKE_COMMAND} -E make_directory ${_MODIR}
            COMMAND ${MSGFMT_COMMAND} --output-file=${_MOFILE} ${_POFILE})
    list(APPEND MOFILES ${_MOFILE})
endforeach ()
add_custom_target(moonlight-i18n ALL DEPENDS ${MOFILES})
add_dependencies(moonlight moonlight-i18n)

file(GLOB_RECURSE I18N_SOURCES LIST_DIRECTORIES FALSE RELATIVE ${CMAKE_SOURCE_DIR} "src/*.c" "src/*.h")
add_custom_target(i18n-update-pot
        COMMAND ${XGETTEXT_COMMAND} --keyword=locstr --keyword=translatable --add-comments -o src/i18n/messages.pot
        ${I18N_SOURCES} WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target(i18n-merge-po
        COMMAND ${MSGMERGE_COMMAND} --update src/i18n/ja/messages.po src/i18n/messages.pot
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
