cmake_minimum_required(VERSION 3.20)
project(simple_com_chart_gui_mfc LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MFC: 1 = use static library
set(CMAKE_MFC_FLAG 1)

add_executable(simple_com_chart_gui_mfc WIN32
    app.rc
    src/mfc_main_dialog.cpp
    src/mfc_app.cpp
    src/mfc_main_dialog.h
    src/resource.h
    src/serial_manager.cpp
    src/serial_manager.h
    src/log_parser.cpp
    src/log_parser.h
    src/channel_model.cpp
    src/channel_model.h
    src/plot_view.cpp
    src/plot_view.h
    src/channel_panel.cpp
    src/channel_panel.h
    src/help_dialog.cpp
    src/help_dialog.h
)

target_include_directories(simple_com_chart_gui_mfc PRIVATE
    src
)

if (MSVC)
    target_compile_definitions(simple_com_chart_gui_mfc PRIVATE UNICODE _UNICODE _WIN32_WINNT=0x0A00)
    target_compile_options(simple_com_chart_gui_mfc PRIVATE /W4 /EHsc)
    # static CRT for portable exe
    target_compile_options(simple_com_chart_gui_mfc PRIVATE /MT)
endif()

target_link_libraries(simple_com_chart_gui_mfc PRIVATE
    comctl32
    setupapi
    gdiplus
)
