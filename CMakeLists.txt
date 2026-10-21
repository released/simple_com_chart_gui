cmake_minimum_required(VERSION 3.20)
project(simple_com_chart_gui_mfc LANGUAGES CXX)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" TOOL_VERSION LIMIT_COUNT 1)
if(NOT TOOL_VERSION MATCHES "^[0-9]\\.[0-9]\\.[0-9]\\.[0-9][0-9][0-9]$")
    message(FATAL_ERROR "VERSION must use a.b.c.ddd")
endif()
string(REPLACE "." ";" VERSION_PARTS "${TOOL_VERSION}")
list(GET VERSION_PARTS 3 VERSION_BUILD)
string(REGEX REPLACE "^0+" "" VERSION_BUILD "${VERSION_BUILD}")
if(VERSION_BUILD STREQUAL "")
    message(FATAL_ERROR "VERSION build component must be 001..999")
endif()
list(REMOVE_AT VERSION_PARTS 3)
list(APPEND VERSION_PARTS "${VERSION_BUILD}")
string(REPLACE ";" "," VERSION_COMMA "${VERSION_PARTS}")
configure_file(src/build_version.h.in generated/build_version.h @ONLY)

# MFC: 1 = use static library
set(CMAKE_MFC_FLAG 1)

add_executable(simple_com_chart_gui_mfc WIN32
    src/SimpleComChart.rc
    src/mfc_main_dialog.cpp
    src/mfc_main_dialog_ui.cpp
    src/mfc_main_dialog_filter.cpp
    src/mfc_main_dialog_export.cpp
    src/capture_store.cpp
    src/capture_export.cpp
    src/data_page.cpp
    src/runtime_smoke.cpp
    src/mfc_app.cpp
    src/mfc_main_dialog.h
    src/resource.h
    src/serial_manager.cpp
    src/serial_manager.h
    src/log_parser.cpp
    src/log_parser.h
    src/line_filter.cpp
    src/line_filter.h
    src/channel_model.cpp
    src/channel_model.h
    src/plot_view.cpp
    src/plot_view_labels.cpp
    src/plot_view.h
    src/channel_panel.cpp
    src/channel_panel.h
    src/help_dialog.cpp
    src/help_dialog.h
)

target_include_directories(simple_com_chart_gui_mfc PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

if (MSVC)
    target_compile_definitions(simple_com_chart_gui_mfc PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
    target_compile_options(simple_com_chart_gui_mfc PRIVATE /W4 /EHsc /utf-8)
endif()

set_target_properties(simple_com_chart_gui_mfc PROPERTIES
    OUTPUT_NAME SimpleComChart
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/build/Debug"
)

target_link_libraries(simple_com_chart_gui_mfc PRIVATE
    comctl32
    setupapi
    gdiplus
    ole32
)

enable_testing()
add_executable(telemetry_tests tests/telemetry_tests.cpp tests/capture_tests.cpp
    src/log_parser.cpp src/line_filter.cpp src/channel_model.cpp src/capture_store.cpp src/capture_export.cpp)
target_include_directories(telemetry_tests PRIVATE src)
if(MSVC)
    target_compile_options(telemetry_tests PRIVATE /W4 /EHsc /utf-8)
endif()
add_test(NAME telemetry_core COMMAND telemetry_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/telemetry.txt")
target_compile_definitions(telemetry_tests PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(telemetry_tests PRIVATE ole32)
