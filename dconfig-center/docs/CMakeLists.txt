find_package (Doxygen REQUIRED)
set (QCH_INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/qt${QT_VERSION_MAJOR}/doc CACHE STRING "QCH install location")

set (DOXYGEN_GENERATE_HTML YES CACHE STRING "Doxygen HTML output")
set (DOXYGEN_GENERATE_XML YES CACHE STRING "Doxygen XML output")
set (DOXYGEN_GENERATE_QHP YES CACHE STRING "Doxygen QHP output")
set (DOXYGEN_FILE_PATTERNS *.cpp *.h *.zh_CN.md *.zh_CN.dox CACHE STRING "Doxygen File Patterns")
set (DOXYGEN_PROJECT_NUMBER ${CMAKE_PROJECT_VERSION} CACHE STRING "") # Should be the same as this project is using.
set (DOXYGEN_EXTRACT_STATIC YES)
set (DOXYGEN_OUTPUT_LANGUAGE "Chinese")
set (DOXYGEN_QCH_FILE "dde-dconfig-doc.qch")
set (DOXYGEN_QHP_VIRTUAL_FOLDER "dde-dconfig-doc")
set (DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/docs/)

if("${QT_VERSION_MAJOR}" STREQUAL "5")
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Help)
else()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ToolsTools)
endif()
get_target_property(_qhelpgenerator_location Qt${QT_VERSION_MAJOR}::qhelpgenerator IMPORTED_LOCATION)

if("${_qhelpgenerator_location}" STREQUAL "")
    set(_qhelpgenerator_location "qhelpgenerator")
endif()
set (DOXYGEN_QHG_LOCATION ${_qhelpgenerator_location} CACHE STRING "Doxygen QHG path")

set (DOXYGEN_HTML_EXTRA_STYLESHEET "" CACHE STRING "Doxygen custom stylesheet for HTML output")
set (DOXYGEN_TAGFILES "qtcore.tags=qthelp://org.qt-project.qtcore/qtcore/" CACHE STRING "Doxygen tag files")
set (DOXYGEN_IMAGE_PATH ${PROJECT_SOURCE_DIR}/docs/src)
set (DOXYGEN_SOURCE_BROWSE "YES")

set (BUILD_THEME OFF CACHE BOOL "Build doxgen theme")
if(BUILD_THEME)
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/doxygen-theme")
message(STATUS "doxygen-theme exists")
else()
execute_process(COMMAND git clone https://github.com/linuxdeepin/doxygen-theme.git --depth=1
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        TIMEOUT 60
    )
execute_process(COMMAND bash themesetting.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/doxygen-theme/
)
endif()
set (DOXYGEN_HTML_EXTRA_STYLESHEET "docs/doxygen-theme/doxygen-awesome-css/doxygen-awesome.css"
                                "docs/doxygen-theme/doxygen-awesome-css/doxygen-awesome-sidebar-only.css"
                                "docs/doxygen-theme/doxygen-awesome-css/doxygen-awesome-sidebar-only-darkmode-toggle.css"
                                )
set (DOXYGEN_HTML_EXTRA_FILES "docs/doxygen-theme/doxygen-awesome-css/doxygen-awesome-darkmode-toggle.js"
                            "docs/doxygen-theme/doxygen-awesome-css/doxygen-awesome-fragment-copy-button.js"
                            "docs/doxygen-theme/doxygen-awesome-css/doxygen-awesome-paragraph-link.js"
                            "docs/doxygen-theme/doxygen-awesome-css/doxygen-awesome-interactive-toc.js"
                            )
set (DOXYGEN_GENERATE_TREEVIEW "YES")
set (DOXYGEN_DISABLE_INDEX "NO")
set (DOXYGEN_FULL_SIDEBAR "NO")
set (DOXYGEN_HTML_HEADER "docs/doxygen-theme/doxygen-awesome-css/header.html")
set (DOXYGEN_HTML_FOOTER "docs/doxygen-theme/doxygen-awesome-css/footer.html")
endif()

set (DOXYGEN_MACRO_EXPANSION "YES")
set (DOXYGEN_EXPAND_ONLY_PREDEF "YES")

doxygen_add_docs (dconfig-doxygen
    ${PROJECT_SOURCE_DIR}/docs
    ALL
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generate documentation via Doxygen"
)

install (FILES ${PROJECT_BINARY_DIR}/docs/html/dde-dconfig-doc.qch DESTINATION ${QCH_INSTALL_DESTINATION})
