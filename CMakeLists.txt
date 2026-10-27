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
option(SCC_ENABLE_RUNTIME_CHECKS "Build the isolated UI verification executable, not the delivery EXE" OFF)
option(SCC_OPTIMIZE_SIZE "Optimize the native Release executable for size without packing" ON)

add_custom_target(copy_ui_languages
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:simple_com_chart_gui_mfc>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/src/i18n/lang_en.ini"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/i18n/lang_tw.ini"
        "$<TARGET_FILE_DIR:simple_com_chart_gui_mfc>"
    COMMENT "Copy portable native language catalogs")

add_executable(simple_com_chart_gui_mfc WIN32
    src/ui_language.cpp
    src/ui_localized_controls.cpp
    src/SimpleComChart.rc
    src/mfc_main_dialog.cpp
    src/mfc_main_dialog_ui.cpp
    src/mfc_main_dialog_filter.cpp
    src/mfc_main_dialog_preferences.cpp
    src/mfc_main_dialog_help.cpp
    src/mfc_main_dialog_serial.cpp
    src/mfc_main_dialog_serial_settings.cpp
    src/serial_connection.cpp
    src/traffic_store.cpp
    src/workbench_session.cpp
    src/uart_workbench.cpp
    src/uart_workbench_layout.cpp
    src/mfc_main_dialog_workbench.cpp
    src/mfc_main_dialog_numeric.cpp
    src/workspace_settings.cpp
    src/binary_options.cpp
    src/binary_inspection.cpp
    src/binary_inspector.cpp
    src/uart_workbench_inspector.cpp
    src/run_analysis.cpp
    src/signal_view.cpp
    src/serial_forwarder.cpp
    src/uart_workbench_forward.cpp
    src/uart_workbench_preferences.cpp
    src/derived_channels.cpp
    src/text_telemetry.cpp
    src/uart_workbench_text.cpp
    src/replay_binary.cpp
    src/mfc_main_dialog_binary_replay.cpp
    src/uart_tool_form.cpp
    src/binary_frame.cpp
    src/uart_workbench_binary.cpp
    src/command_panel.cpp
    src/step_script.cpp
    src/tx_pacing.cpp
    src/uart_workbench_tools.cpp
    src/uart_command_window.cpp
    src/block_protocol.cpp
    src/transfer_job.cpp
    src/uart_workbench_transfer.cpp
    src/terminal_screen.cpp
    src/modbus_rtu.cpp
    src/uart_workbench_rtu.cpp
    src/signal_analysis.cpp
    src/uart_instruments.cpp
    src/traffic_framer.cpp
    src/uart_workbench_framing.cpp
    src/serial_open_job.cpp
    src/hover_help.cpp
    src/mfc_main_dialog_export.cpp
    src/capture_store.cpp
    src/capture_export.cpp
    src/data_page.cpp
    src/data_page_table.cpp
    src/message_buffer.cpp
    src/message_format.cpp
    src/message_page.cpp
    src/message_page_options.cpp
    src/uart_recorder.cpp
    src/mfc_main_dialog_recording.cpp
    src/uart_operations.cpp
    src/uart_preferences.cpp
    src/uart_console.cpp
    src/mfc_main_dialog_uart.cpp
    src/serial_control_config.cpp
    src/serial_manager_operations.cpp
    src/serial_native_io.cpp
    src/uart_files.cpp
    src/uart_offline.cpp
    src/replay_storage.cpp
    src/file_transfer.cpp
    src/mfc_main_dialog_file_tx.cpp
    src/pending_data.cpp
    src/uart_profiles.cpp
    src/uart_profile_ini.cpp
    src/uart_automation.cpp
    src/chart_analysis.cpp
    src/advanced_tools.cpp
    src/plot_view_analysis.cpp
    src/plot_view_navigation.cpp
    src/mfc_main_dialog_advanced.cpp
    src/mfc_main_dialog_replay.cpp
    src/mfc_main_dialog_profiles.cpp
    src/mfc_main_dialog_automation.cpp
    src/mfc_app.cpp
    src/session_instance.cpp
    src/mfc_main_dialog_dpi.cpp
    src/field_extractor.cpp
    src/mfc_main_dialog_extraction.cpp
    src/display_preferences.cpp
    src/mfc_main_dialog_channel_display.cpp
    src/source_mapping.cpp
    src/mfc_main_dialog_navigation.cpp
    src/message_page_navigation.cpp
    src/light_terminal.cpp
    src/message_page_terminal.cpp
    src/mfc_main_dialog_terminal.cpp
    src/mfc_main_dialog.h
    src/resource.h
    src/serial_manager.cpp
    src/serial_manager.h
    src/log_parser.cpp
    src/bounded_regex.cpp
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

if(SCC_ENABLE_RUNTIME_CHECKS)
    target_sources(simple_com_chart_gui_mfc PRIVATE
        src/runtime_test_telemetry.cpp
        src/runtime_workbench.cpp
        src/runtime_analysis_expansion.cpp
        src/runtime_uart_lab.cpp
        src/runtime_uart_session.cpp
        src/runtime_ui_language.cpp
        src/runtime_workflows.cpp
        src/runtime_message_log.cpp
        src/runtime_optimization.cpp src/runtime_receive_soak.cpp src/runtime_smoke.cpp
        src/runtime_messages.cpp src/runtime_uart_receive.cpp src/runtime_uart_operations.cpp
        src/runtime_advanced_tools.cpp src/runtime_message_export.cpp src/runtime_message_font.cpp
        src/runtime_serial.cpp src/runtime_connection_redraw.cpp src/runtime_startup_tab.cpp
        src/runtime_preferences.cpp src/runtime_buttons.cpp src/runtime_ui_help.cpp
    )
    target_compile_definitions(simple_com_chart_gui_mfc PRIVATE CHART_RUNTIME_CHECKS=1)
endif()

target_include_directories(simple_com_chart_gui_mfc PRIVATE
    src
    "${CMAKE_CURRENT_BINARY_DIR}/generated"
)

if (MSVC)
    target_compile_definitions(simple_com_chart_gui_mfc PRIVATE UNICODE _UNICODE NOMINMAX _WIN32_WINNT=0x0A00)
    # Only Win32/common/custom controls are used. Exclude automatic Feature Pack
    # control registration; all application dialogs must remain CDialog-based.
    target_compile_definitions(simple_com_chart_gui_mfc PRIVATE _AFX_NO_MFC_CONTROLS_IN_DIALOGS)
    target_compile_options(simple_com_chart_gui_mfc PRIVATE /W4 /EHsc /utf-8)
    target_link_options(simple_com_chart_gui_mfc PRIVATE "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/SimpleComChart.manifest")
    target_link_options(simple_com_chart_gui_mfc PRIVATE "/MAP:${CMAKE_CURRENT_BINARY_DIR}/SimpleComChart-$<CONFIG>.map")
    if(SCC_OPTIMIZE_SIZE)
        target_compile_options(simple_com_chart_gui_mfc PRIVATE "$<$<CONFIG:Release>:/O1;/GL;/Gw>")
        target_link_options(simple_com_chart_gui_mfc PRIVATE "$<$<CONFIG:Release>:/LTCG;/OPT:REF;/OPT:ICF;/INCREMENTAL:NO>")
    endif()
endif()

set_target_properties(simple_com_chart_gui_mfc PROPERTIES
    VS_DPI_AWARE ON
    OUTPUT_NAME SimpleComChart
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/build/Debug"
)
if(SCC_ENABLE_RUNTIME_CHECKS)
    # A verification build must never overwrite the portable delivery executable.
    set_target_properties(simple_com_chart_gui_mfc PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/runtime-bin/Release"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/runtime-bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/runtime-bin/MinSizeRel"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/runtime-bin/RelWithDebInfo"
    )
endif()

target_link_libraries(simple_com_chart_gui_mfc PRIVATE
    comctl32
    setupapi
    gdiplus
    ole32
)
target_sources(simple_com_chart_gui_mfc PRIVATE src/packet_builder.cpp src/lab_metrics.cpp
    src/digital_history.cpp
    src/recording_tools.cpp src/digital_view.cpp src/uart_lab_window.cpp src/uart_lab_pages.cpp
    src/mfc_main_dialog_lab.cpp src/mfc_main_dialog_lab_files.cpp src/mfc_main_dialog_lab_packet.cpp
    src/mfc_main_dialog_lab_parser.cpp src/mfc_main_dialog_lab_analysis.cpp)

enable_testing()
add_executable(telemetry_tests tests/telemetry_tests.cpp tests/capture_tests.cpp tests/message_tests.cpp src/message_buffer.cpp src/message_format.cpp
    src/log_parser.cpp src/line_filter.cpp src/channel_model.cpp src/capture_store.cpp src/capture_export.cpp src/serial_connection.cpp
    src/serial_open_job.cpp tests/serial_connection_tests.cpp src/uart_operations.cpp)
target_include_directories(telemetry_tests PRIVATE src)
if(MSVC)
    target_compile_options(telemetry_tests PRIVATE /W4 /EHsc /utf-8)
endif()
add_test(NAME telemetry_core COMMAND telemetry_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/telemetry.txt")
target_compile_definitions(telemetry_tests PRIVATE UNICODE _UNICODE NOMINMAX)
target_link_libraries(telemetry_tests PRIVATE ole32)

add_executable(uart_receive_tests tests/uart_receive_tests.cpp src/message_buffer.cpp src/message_format.cpp src/uart_recorder.cpp)
target_include_directories(uart_receive_tests PRIVATE src)
target_compile_definitions(uart_receive_tests PRIVATE UNICODE _UNICODE NOMINMAX)
if(MSVC)
    target_compile_options(uart_receive_tests PRIVATE /W4 /EHsc /utf-8)
endif()
add_test(NAME uart_receive COMMAND uart_receive_tests "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/uart-core")

add_executable(uart_operation_tests tests/uart_operation_tests.cpp tests/uart_native_io_tests.cpp src/serial_native_io.cpp src/uart_operations.cpp src/uart_preferences.cpp src/serial_control_config.cpp src/serial_open_job.cpp src/serial_connection.cpp)
target_include_directories(uart_operation_tests PRIVATE src)
target_compile_definitions(uart_operation_tests PRIVATE UNICODE _UNICODE NOMINMAX)
if(MSVC)
    target_compile_options(uart_operation_tests PRIVATE /W4 /EHsc /utf-8)
endif()
add_test(NAME uart_operations COMMAND uart_operation_tests "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/uart-operations-core")

add_executable(uart_workbench_tests tests/uart_workbench_tests.cpp src/traffic_store.cpp src/workbench_session.cpp
    src/serial_open_job.cpp src/serial_connection.cpp src/serial_manager.cpp src/serial_manager_operations.cpp
    src/serial_native_io.cpp src/serial_control_config.cpp src/uart_operations.cpp src/uart_recorder.cpp src/message_format.cpp)
target_include_directories(uart_workbench_tests PRIVATE src)
target_compile_definitions(uart_workbench_tests PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_options(uart_workbench_tests PRIVATE /W4 /EHsc /utf-8)
target_link_libraries(uart_workbench_tests PRIVATE setupapi)
add_test(NAME uart_workbench COMMAND uart_workbench_tests "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/workbench-core")

add_executable(binary_frame_tests tests/binary_frame_tests.cpp src/binary_frame.cpp)
target_include_directories(binary_frame_tests PRIVATE src)
target_compile_definitions(binary_frame_tests PRIVATE NOMINMAX)
target_compile_options(binary_frame_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME binary_frames COMMAND binary_frame_tests)

add_executable(uart_command_tests tests/uart_command_tests.cpp src/command_panel.cpp src/step_script.cpp src/tx_pacing.cpp src/binary_frame.cpp src/uart_files.cpp src/uart_operations.cpp)
target_include_directories(uart_command_tests PRIVATE src)
target_compile_definitions(uart_command_tests PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_options(uart_command_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME uart_commands COMMAND uart_command_tests)
target_sources(uart_command_tests PRIVATE src/log_parser.cpp src/source_mapping.cpp)
target_sources(uart_command_tests PRIVATE src/message_format.cpp)

add_executable(block_protocol_tests tests/block_protocol_tests.cpp src/block_protocol.cpp)
target_include_directories(block_protocol_tests PRIVATE src)
target_compile_definitions(block_protocol_tests PRIVATE NOMINMAX)
target_compile_options(block_protocol_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME block_protocol COMMAND block_protocol_tests)

add_executable(uart_protocol_display_tests tests/uart_protocol_display_tests.cpp src/modbus_rtu.cpp src/terminal_screen.cpp src/signal_analysis.cpp src/binary_frame.cpp src/command_panel.cpp src/uart_files.cpp src/uart_operations.cpp src/tx_pacing.cpp src/traffic_framer.cpp src/traffic_store.cpp)
target_include_directories(uart_protocol_display_tests PRIVATE src)
target_compile_definitions(uart_protocol_display_tests PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(uart_protocol_display_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME uart_protocol_display COMMAND uart_protocol_display_tests)
target_sources(uart_protocol_display_tests PRIVATE src/source_mapping.cpp)
target_sources(uart_protocol_display_tests PRIVATE src/message_format.cpp)

add_executable(transfer_job_tests tests/transfer_job_tests.cpp src/transfer_job.cpp src/block_protocol.cpp src/traffic_store.cpp src/uart_operations.cpp src/uart_files.cpp src/tx_pacing.cpp src/message_format.cpp)
target_include_directories(transfer_job_tests PRIVATE src)
target_compile_definitions(transfer_job_tests PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(transfer_job_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME transfer_job COMMAND transfer_job_tests "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/transfer-core")

add_executable(uart_advanced_tests tests/uart_advanced_tests.cpp src/uart_files.cpp src/uart_offline.cpp src/uart_profiles.cpp src/uart_profile_ini.cpp src/uart_automation.cpp src/chart_analysis.cpp src/capture_store.cpp src/line_filter.cpp src/message_format.cpp src/serial_connection.cpp src/uart_operations.cpp src/uart_preferences.cpp)
target_include_directories(uart_advanced_tests PRIVATE src)
target_compile_definitions(uart_advanced_tests PRIVATE UNICODE _UNICODE NOMINMAX)
if(MSVC)
    target_compile_options(uart_advanced_tests PRIVATE /W4 /EHsc /utf-8)
endif()
add_test(NAME uart_advanced COMMAND uart_advanced_tests "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/uart-advanced-core")
target_sources(telemetry_tests PRIVATE src/bounded_regex.cpp)
target_sources(telemetry_tests PRIVATE src/display_preferences.cpp src/uart_files.cpp)
target_sources(uart_advanced_tests PRIVATE src/bounded_regex.cpp)
target_sources(uart_advanced_tests PRIVATE src/field_extractor.cpp src/log_parser.cpp)
target_sources(uart_advanced_tests PRIVATE src/replay_storage.cpp)
target_sources(uart_advanced_tests PRIVATE src/display_preferences.cpp)
target_sources(uart_advanced_tests PRIVATE src/workspace_settings.cpp src/binary_options.cpp src/binary_frame.cpp src/derived_channels.cpp src/text_telemetry.cpp src/source_mapping.cpp src/traffic_framer.cpp)

add_executable(uart_optimization_tests tests/uart_optimization_tests.cpp src/bounded_regex.cpp src/line_filter.cpp src/field_extractor.cpp src/log_parser.cpp src/uart_files.cpp src/message_format.cpp)
target_include_directories(uart_optimization_tests PRIVATE src)
target_compile_definitions(uart_optimization_tests PRIVATE UNICODE _UNICODE NOMINMAX)
if(MSVC)
    target_compile_options(uart_optimization_tests PRIVATE /W4 /EHsc /utf-8)
endif()
add_test(NAME uart_optimization COMMAND uart_optimization_tests "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/uart-optimization-core")
target_sources(uart_optimization_tests PRIVATE src/display_preferences.cpp)
target_sources(uart_optimization_tests PRIVATE src/source_mapping.cpp)
target_sources(uart_optimization_tests PRIVATE src/light_terminal.cpp)
target_sources(uart_optimization_tests PRIVATE src/replay_storage.cpp src/uart_offline.cpp src/uart_operations.cpp)
target_sources(uart_optimization_tests PRIVATE src/file_transfer.cpp)

add_executable(session_instance_tests tests/session_instance_tests.cpp src/session_instance.cpp)
target_include_directories(session_instance_tests PRIVATE src)
target_compile_definitions(session_instance_tests PRIVATE UNICODE _UNICODE NOMINMAX)
if(MSVC)
    target_compile_options(session_instance_tests PRIVATE /W4 /EHsc /utf-8)
endif()
add_test(NAME session_instance COMMAND session_instance_tests "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/session-instance")

foreach(target telemetry_tests uart_operation_tests uart_workbench_tests uart_advanced_tests uart_optimization_tests)
    target_sources(${target} PRIVATE src/tx_pacing.cpp)
endforeach()

# Native workflow features reuse the existing static Windows/MFC architecture.
target_sources(simple_com_chart_gui_mfc PRIVATE src/message_log_controls.cpp src/workflow_views.cpp src/workflow_notes.cpp
    src/workflow_profile_diff.cpp src/workflow_report.cpp src/workflow_waterfall.cpp
    src/workflow_png.cpp src/workflow_grid.cpp src/waterfall_view.cpp
    src/mfc_main_dialog_views.cpp src/mfc_main_dialog_notes.cpp
    src/mfc_main_dialog_workflow_report.cpp src/mfc_main_dialog_waterfall.cpp)


add_executable(uart_analysis_tests tests/uart_analysis_tests.cpp src/text_telemetry.cpp src/source_mapping.cpp src/field_extractor.cpp src/line_filter.cpp src/bounded_regex.cpp src/log_parser.cpp src/uart_files.cpp src/message_format.cpp)
target_include_directories(uart_analysis_tests PRIVATE src)
target_compile_definitions(uart_analysis_tests PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(uart_analysis_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME uart_analysis COMMAND uart_analysis_tests)
target_sources(uart_analysis_tests PRIVATE src/replay_binary.cpp src/binary_frame.cpp)
target_sources(uart_analysis_tests PRIVATE src/derived_channels.cpp)
target_sources(uart_analysis_tests PRIVATE src/binary_inspection.cpp)
target_sources(uart_analysis_tests PRIVATE src/command_panel.cpp src/uart_operations.cpp src/tx_pacing.cpp)
target_sources(uart_analysis_tests PRIVATE src/run_analysis.cpp src/capture_store.cpp src/uart_offline.cpp src/replay_storage.cpp)
target_sources(uart_analysis_tests PRIVATE src/serial_forwarder.cpp)

add_executable(plot_render_benchmark tests/plot_render_benchmark.cpp)
target_include_directories(plot_render_benchmark PRIVATE src)
target_compile_definitions(plot_render_benchmark PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(plot_render_benchmark PRIVATE /W4 /EHsc /utf-8)
target_link_libraries(plot_render_benchmark PRIVATE gdiplus)

# Share the bounded native codec/layout implementation with every binary consumer.
get_property(scc_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(scc_target IN LISTS scc_targets)
    get_target_property(scc_sources ${scc_target} SOURCES)
    if("src/binary_frame.cpp" IN_LIST scc_sources)
        target_sources(${scc_target} PRIVATE src/wire_codec.cpp src/binary_layout.cpp)
    endif()
    if("src/text_telemetry.cpp" IN_LIST scc_sources)
        target_sources(${scc_target} PRIVATE src/pipeline_diagnostics.cpp)
    endif()
    if("src/workspace_settings.cpp" IN_LIST scc_sources)
        foreach(scc_dependency src/lab_metrics.cpp src/command_panel.cpp)
            if(NOT scc_dependency IN_LIST scc_sources)
                target_sources(${scc_target} PRIVATE ${scc_dependency})
            endif()
        endforeach()
    endif()
endforeach()

add_executable(uart_lab_tests tests/uart_lab_tests.cpp src/packet_builder.cpp src/lab_metrics.cpp
    src/command_panel.cpp src/binary_frame.cpp src/binary_layout.cpp src/wire_codec.cpp
    src/field_extractor.cpp src/bounded_regex.cpp src/log_parser.cpp src/uart_files.cpp
    src/uart_operations.cpp src/tx_pacing.cpp src/message_format.cpp)
target_sources(uart_lab_tests PRIVATE src/pipeline_diagnostics.cpp src/recording_tools.cpp src/uart_offline.cpp src/replay_storage.cpp)
target_sources(uart_lab_tests PRIVATE src/capture_store.cpp src/digital_history.cpp src/line_filter.cpp)
target_sources(uart_lab_tests PRIVATE src/binary_inspection.cpp)
target_include_directories(uart_lab_tests PRIVATE src)
target_compile_definitions(uart_lab_tests PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(uart_lab_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME uart_lab COMMAND uart_lab_tests)

add_executable(simple_com_chart_cli src/uart_cli.cpp src/uart_cli.rc src/finite_script.cpp
    src/step_script.cpp src/command_panel.cpp src/source_mapping.cpp src/log_parser.cpp
    src/binary_frame.cpp src/binary_layout.cpp src/wire_codec.cpp src/uart_files.cpp
    src/message_format.cpp src/uart_operations.cpp src/tx_pacing.cpp
    src/serial_manager.cpp src/serial_manager_operations.cpp src/serial_native_io.cpp
    src/serial_control_config.cpp src/serial_connection.cpp)
target_include_directories(simple_com_chart_cli PRIVATE src "${CMAKE_CURRENT_BINARY_DIR}/generated")
target_compile_definitions(simple_com_chart_cli PRIVATE NOMINMAX UNICODE _UNICODE _WIN32_WINNT=0x0A00)
target_compile_options(simple_com_chart_cli PRIVATE /W4 /EHsc /utf-8)
target_link_libraries(simple_com_chart_cli PRIVATE setupapi)
if(SCC_OPTIMIZE_SIZE)
    target_compile_options(simple_com_chart_cli PRIVATE "$<$<CONFIG:Release>:/O1;/GL;/Gw>")
    target_link_options(simple_com_chart_cli PRIVATE "$<$<CONFIG:Release>:/LTCG;/OPT:REF;/OPT:ICF;/INCREMENTAL:NO>")
endif()
set_target_properties(simple_com_chart_cli PROPERTIES OUTPUT_NAME SimpleComChartCli
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/build"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/build/Debug")
if(SCC_ENABLE_RUNTIME_CHECKS)
    set_target_properties(simple_com_chart_cli PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/runtime-bin/Release"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_CURRENT_SOURCE_DIR}/build/verify/runtime-bin/Debug")
endif()
target_sources(uart_lab_tests PRIVATE tests/uart_lab_files_tests.cpp src/finite_script.cpp src/step_script.cpp src/source_mapping.cpp)

add_executable(uart_lab_performance tests/uart_lab_performance.cpp src/line_filter.cpp
    src/field_extractor.cpp src/bounded_regex.cpp src/log_parser.cpp src/pipeline_diagnostics.cpp src/uart_files.cpp)
target_include_directories(uart_lab_performance PRIVATE src)
target_compile_definitions(uart_lab_performance PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(uart_lab_performance PRIVATE /W4 /EHsc /utf-8 "$<$<CONFIG:Release>:/O1;/GL;/Gw>")
target_link_options(uart_lab_performance PRIVATE "$<$<CONFIG:Release>:/LTCG;/OPT:REF;/OPT:ICF;/INCREMENTAL:NO>")
add_test(NAME uart_lab_performance COMMAND uart_lab_performance)

target_sources(simple_com_chart_gui_mfc PRIVATE src/recording_settings.cpp src/input_framing.cpp
    src/device_catalog.cpp src/session_metadata.cpp src/session_manifest.cpp
    src/time_calculus.cpp src/calculus_view.cpp src/transaction_analysis.cpp
    src/mfc_main_dialog_lab_session.cpp src/mfc_main_dialog_lab_options.cpp
    src/mfc_main_dialog_lab_calculus.cpp src/mfc_main_dialog_lab_transactions.cpp)
target_link_libraries(simple_com_chart_gui_mfc PRIVATE bcrypt advapi32)
add_dependencies(simple_com_chart_gui_mfc copy_ui_languages)
get_property(scc_session_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(scc_target IN LISTS scc_session_targets)
    get_target_property(scc_sources ${scc_target} SOURCES)
    if("src/uart_recorder.cpp" IN_LIST scc_sources)
        target_sources(${scc_target} PRIVATE src/recording_limits.cpp)
    endif()
endforeach()
add_executable(uart_session_tests tests/uart_session_tests.cpp src/uart_recorder.cpp
    src/recording_limits.cpp src/message_format.cpp src/uart_files.cpp)
target_include_directories(uart_session_tests PRIVATE src)
target_compile_definitions(uart_session_tests PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(uart_session_tests PRIVATE /W4 /EHsc /utf-8)
add_test(NAME uart_session COMMAND uart_session_tests)
target_sources(uart_session_tests PRIVATE tests/uart_session_algorithms_tests.cpp
    src/field_extractor.cpp src/bounded_regex.cpp src/log_parser.cpp src/source_mapping.cpp src/line_filter.cpp
    src/packet_builder.cpp src/command_panel.cpp src/binary_frame.cpp src/binary_layout.cpp src/wire_codec.cpp
    src/uart_operations.cpp src/tx_pacing.cpp src/time_calculus.cpp src/capture_store.cpp src/device_catalog.cpp
    src/session_metadata.cpp src/uart_profiles.cpp src/workspace_settings.cpp src/binary_options.cpp
    src/derived_channels.cpp src/text_telemetry.cpp src/traffic_framer.cpp src/lab_metrics.cpp
    src/pipeline_diagnostics.cpp src/display_preferences.cpp src/serial_connection.cpp)
target_sources(uart_session_tests PRIVATE tests/uart_session_files_tests.cpp
    src/session_manifest.cpp src/uart_offline.cpp src/replay_storage.cpp
    src/transaction_analysis.cpp src/traffic_store.cpp)
target_link_libraries(uart_session_tests PRIVATE setupapi advapi32 bcrypt)
add_executable(ui_language_tests tests/ui_language_tests.cpp src/ui_language.cpp
    src/ui_localized_controls.cpp src/uart_files.cpp src/message_format.cpp)
target_include_directories(ui_language_tests PRIVATE src)
target_compile_definitions(ui_language_tests PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(ui_language_tests PRIVATE /W4 /EHsc /utf-8)
target_link_libraries(ui_language_tests PRIVATE comctl32)
add_test(NAME ui_language COMMAND ui_language_tests)
get_property(scc_session_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(scc_target IN LISTS scc_session_targets)
    get_target_property(scc_sources ${scc_target} SOURCES)
    if("src/binary_frame.cpp" IN_LIST scc_sources)
        target_sources(${scc_target} PRIVATE src/custom_crc.cpp)
    endif()
    if("src/field_extractor.cpp" IN_LIST scc_sources)
        target_sources(${scc_target} PRIVATE src/json_numeric.cpp)
    endif()
    if("src/workspace_settings.cpp" IN_LIST scc_sources AND NOT "src/recording_limits.cpp" IN_LIST scc_sources)
        target_sources(${scc_target} PRIVATE src/recording_limits.cpp)
    endif()
endforeach()

get_property(scc_digest_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(scc_target IN LISTS scc_digest_targets)
    get_target_property(scc_sources ${scc_target} SOURCES)
    if("src/session_manifest.cpp" IN_LIST scc_sources)
        target_sources(${scc_target} PRIVATE src/file_digest.cpp)
    endif()
endforeach()

get_target_property(scc_workflow_core uart_session_tests SOURCES)
list(FILTER scc_workflow_core INCLUDE REGEX "^src/")
add_executable(uart_workflow_tests tests/uart_workflow_tests.cpp ${scc_workflow_core}
    src/workflow_views.cpp src/workflow_notes.cpp src/workflow_profile_diff.cpp
    src/workflow_report.cpp src/workflow_waterfall.cpp src/chart_analysis.cpp)
target_include_directories(uart_workflow_tests PRIVATE src)
target_compile_definitions(uart_workflow_tests PRIVATE NOMINMAX UNICODE _UNICODE)
target_compile_options(uart_workflow_tests PRIVATE /W4 /EHsc /utf-8)
target_link_libraries(uart_workflow_tests PRIVATE setupapi advapi32 bcrypt)
add_test(NAME uart_workflow COMMAND uart_workflow_tests)
