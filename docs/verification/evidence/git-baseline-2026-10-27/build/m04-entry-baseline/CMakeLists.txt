cmake_minimum_required(VERSION 3.25)
project(RealityEngine LANGUAGES CXX)

include(CTest)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)

if(NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "M00 currently supports the documented MSVC x64 toolchain only.")
endif()

option(REALITY_ENABLE_ASAN "Instrument the M00 build with AddressSanitizer" OFF)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(reality_build_options INTERFACE)
target_compile_features(reality_build_options INTERFACE cxx_std_20)
target_compile_options(reality_build_options INTERFACE
    /W4 /WX /permissive- /utf-8 /Zc:__cplusplus
)

if(REALITY_ENABLE_ASAN)
    if("Debug" IN_LIST CMAKE_CONFIGURATION_TYPES)
        message(FATAL_ERROR "Use windows-asan: its RelWithDebInfo configuration avoids /RTC conflicts.")
    endif()
    target_compile_options(reality_build_options INTERFACE /fsanitize=address /Zi)
    target_link_options(reality_build_options INTERFACE /INCREMENTAL:NO)
    get_filename_component(reality_compiler_directory "${CMAKE_CXX_COMPILER}" DIRECTORY)
    find_file(REALITY_ASAN_RUNTIME
        NAMES clang_rt.asan_dynamic-x86_64.dll
        HINTS "${reality_compiler_directory}"
        NO_DEFAULT_PATH REQUIRED
    )
endif()

function(reality_deploy_diagnostics target)
    if(REALITY_ENABLE_ASAN)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${REALITY_ASAN_RUNTIME}" "$<TARGET_FILE_DIR:${target}>"
            VERBATIM
        )
    endif()
endfunction()

add_library(reality_core STATIC
    src/core/simulation_clock.cpp
    src/core/entity_registry.cpp
    src/simulation/work_world.cpp
    src/persistence/world_snapshot.cpp
)
target_include_directories(reality_core PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(reality_core PUBLIC reality_build_options)

add_executable(reality_headless src/headless/main.cpp)
target_link_libraries(reality_headless PRIVATE reality_core)
reality_deploy_diagnostics(reality_headless)

add_executable(reality_work_demo src/headless/work_demo.cpp)
target_link_libraries(reality_work_demo PRIVATE reality_core)
reality_deploy_diagnostics(reality_work_demo)

add_library(reality_lab_model STATIC src/lab/lab_session.cpp src/graphics/lab_scene.cpp
    src/physics/kinematic.cpp src/navigation/grid_navigation.cpp src/lab/motion_session.cpp
    src/physics/spatial_geometry.cpp src/navigation/surface_navigation.cpp src/physics/locomotion.cpp
    src/physics/platform_motion.cpp src/lab/traversal_session.cpp src/lab/traversal_course.cpp
    src/lab/traversal_codec.cpp src/graphics/traversal_scene.cpp)
target_link_libraries(reality_lab_model PUBLIC reality_core)

add_library(reality_graphics STATIC src/graphics/d3d11_renderer.cpp)
target_link_libraries(reality_graphics PUBLIC reality_lab_model d3d11 dxgi d3dcompiler windowscodecs ole32)
target_compile_definitions(reality_graphics PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

add_executable(reality_lab WIN32 src/platform/lab_main.cpp)
target_link_libraries(reality_lab PRIVATE reality_graphics user32 gdi32 shell32)
reality_deploy_diagnostics(reality_lab)
add_executable(reality_traversal WIN32 src/platform/traversal_main.cpp)
target_link_libraries(reality_traversal PRIVATE reality_graphics user32 gdi32 shell32)
reality_deploy_diagnostics(reality_traversal)

set(reality_cpp_files
    src/core/simulation_clock.hpp
    src/core/simulation_clock.cpp
    src/headless/main.cpp
    src/core/entity_registry.hpp
    src/core/entity_registry.cpp
    src/simulation/work_world.hpp
    src/simulation/work_world.cpp
    src/persistence/world_snapshot.hpp
    src/persistence/world_snapshot.cpp
    src/headless/work_demo.cpp
    tests/clock_tests.cpp
    tests/work_world_tests.cpp
    tests/diagnostics/asan_probe.cpp
    src/lab/lab_session.hpp
    src/lab/lab_session.cpp
    src/graphics/lab_scene.hpp
    src/graphics/lab_scene.cpp
    src/graphics/d3d11_renderer.hpp
    src/graphics/d3d11_renderer.cpp
    src/platform/lab_main.cpp
    tests/lab_tests.cpp
    src/physics/kinematic.hpp
    src/physics/kinematic.cpp
    src/navigation/grid_navigation.hpp
    src/navigation/grid_navigation.cpp
    src/lab/motion_session.hpp
    src/lab/motion_session.cpp
    tests/motion_tests.cpp
    tests/motion_adversarial_tests.cpp
    src/physics/spatial_geometry.hpp
    src/physics/spatial_geometry.cpp
    tests/spatial_tests.cpp
    src/navigation/surface_navigation.hpp
    src/navigation/surface_navigation.cpp
    src/physics/locomotion.hpp
    src/physics/locomotion.cpp
    src/physics/platform_motion.hpp
    src/physics/platform_motion.cpp
    src/lab/traversal_session.hpp
    src/lab/traversal_session.cpp
    src/lab/traversal_course.cpp
    src/lab/traversal_codec.cpp
    src/graphics/traversal_scene.cpp
    src/platform/traversal_main.cpp
    tests/traversal_tests.cpp
    tests/traversal_audit_tests.cpp
)
list(TRANSFORM reality_cpp_files PREPEND "${CMAKE_CURRENT_SOURCE_DIR}/")
find_program(REALITY_CLANG_FORMAT NAMES clang-format
    HINTS "${CMAKE_GENERATOR_INSTANCE}/VC/Tools/Llvm/bin" REQUIRED
)
add_custom_target(format-check
    COMMAND "${REALITY_CLANG_FORMAT}" --dry-run --Werror ${reality_cpp_files}
    VERBATIM
)

if(BUILD_TESTING)
    add_executable(reality_traversal_audit_tests tests/traversal_audit_tests.cpp)
    target_link_libraries(reality_traversal_audit_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_traversal_audit_tests)
    foreach(case IN ITEMS narrow_bridge_opposite ferry_exchange rapid_retarget obstacle_braking
            slope_maneuvers rotating_support crowd_passing occupied_goal_recovery snapshot_corruption long_course partition
            collinear_rest moving_passengers command_isolation support_contract moving_contact
            wall_neighbour_recovery blocked_route_settling occupied_queue_parking
            wall_neighbour_matrix_0 wall_neighbour_matrix_1 wall_neighbour_matrix_2
            wall_neighbour_matrix_3 wall_neighbour_matrix_4 wall_neighbour_matrix_5)
        add_test(NAME traversal.${case} COMMAND reality_traversal_audit_tests --case ${case})
    endforeach()
    add_executable(reality_traversal_tests tests/traversal_tests.cpp)
    target_link_libraries(reality_traversal_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_traversal_tests)
    foreach(case IN ITEMS course walk ramp step rider autonomy occupied_goal transport
            supersede_pending_goal supersede_pending_stop release_passage_ownership manual_touching_departure
            command_roundtrip support_coverage need_fallback_empty need_fallback_unreachable last_meal reusable_rest)
        add_test(NAME traversal.${case} COMMAND reality_traversal_tests --case ${case})
    endforeach()
    add_executable(reality_spatial_tests tests/spatial_tests.cpp)
    target_link_libraries(reality_spatial_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_spatial_tests)
    foreach(case IN ITEMS ramp rotated_sweep levels platform_volume solid_sat invalid_data
            surface_narrow surface_metric surface_step surface_budgets sphere_sweep
            locomotion_envelopes locomotion_arrival locomotion_turn pair_paths unequal_capsules)
        add_test(NAME spatial.${case} COMMAND reality_spatial_tests --case ${case})
    endforeach()
    add_executable(reality_adversarial_tests tests/motion_adversarial_tests.cpp)
    target_link_libraries(reality_adversarial_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_adversarial_tests)
    foreach(case IN ITEMS wall_retarget wall_goal stationary_blocker extreme_span interpolation corner_image
            rapid_targets passing goal_swap collision_sequences input_limits legacy_endpoint contact_rounding)
        add_test(NAME adversarial.${case} COMMAND reality_adversarial_tests --case ${case})
    endforeach()
    add_executable(reality_motion_tests tests/motion_tests.cpp)
    target_link_libraries(reality_motion_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_motion_tests)
    foreach(case IN ITEMS sweep corners size_height invalid_data route_oracle route_failures route_stamps
            detour_wait gate_occupancy input_collision persistence partition waiting_continuation
            direct_goal retarget replan_forward arrival_stability shortcut_geometry retarget_continuation)
        add_test(NAME motion.${case} COMMAND reality_motion_tests --case ${case})
    endforeach()
    add_test(NAME motion.hardware_gpu_continuation
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/motion_gpu_integration.py"
            "$<TARGET_FILE:reality_lab>" "${CMAKE_CURRENT_SOURCE_DIR}/tests/data/m03-v1-inflight.save"
    )
    add_executable(reality_lab_tests tests/lab_tests.cpp)
    target_link_libraries(reality_lab_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_lab_tests)
    add_test(NAME lab.model_and_presentation COMMAND reality_lab_tests)
    add_test(NAME lab.hardware_gpu_integration
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/lab_gpu_integration.py"
            "$<TARGET_FILE:reality_lab>"
    )
    add_test(NAME lab.hardware_gpu_standard
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/lab_gpu_integration.py"
            "$<TARGET_FILE:reality_lab>" --standard
    )
    add_executable(reality_tests tests/clock_tests.cpp)
    target_link_libraries(reality_tests PRIVATE reality_core)
    reality_deploy_diagnostics(reality_tests)

    add_executable(reality_work_tests tests/work_world_tests.cpp)
    target_link_libraries(reality_work_tests PRIVATE reality_core)
    reality_deploy_diagnostics(reality_work_tests)
    foreach(case IN ITEMS stale_handle registry_exhaustion registry_validation
            reservation_contention command_retries cancel_before_start partial_cancellation
            completion_once participant_removal invalid_requests time_overflow
            snapshot_continuation snapshot_history snapshot_rejection snapshot_semantics
            snapshot_journal save_replacement record_limit event_capacity batch_ordering operation_sequence
            capacity_lifecycle capacity_removal capacity_snapshot)
        add_test(NAME work.${case} COMMAND reality_work_tests --case ${case})
    endforeach()
    add_test(NAME work.fresh_process_continuation
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/work_demo_integration.py"
            "$<TARGET_FILE:reality_work_demo>"
    )

    foreach(case IN ITEMS initial_state reject_zero_step reject_negative_step exact_elapsed_time
            batch_equivalence zero_batch overflow_preserves_state reject_large_batch)
        add_test(NAME clock.${case} COMMAND reality_tests --case ${case})
    endforeach()

    set(reality_process_check "${CMAKE_CURRENT_SOURCE_DIR}/tools/expect_process.py")
    add_test(NAME work_runner.rejects_failed_assertion
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "INTENTIONAL WORK TEST FAILURE"
            -- "$<TARGET_FILE:reality_work_tests>" --self-test-failure
    )
    add_test(NAME work_runner.rejects_unknown_case
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "unknown test case"
            -- "$<TARGET_FILE:reality_work_tests>" --case absent
    )
    add_test(NAME runner.rejects_failed_assertion
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "INTENTIONAL TEST FAILURE"
            -- "$<TARGET_FILE:reality_tests>" --self-test-failure
    )
    add_test(NAME runner.rejects_unknown_case
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "unknown test case"
            -- "$<TARGET_FILE:reality_tests>" --case absent
    )
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/empty-suite")
    add_test(NAME runner.rejects_empty_suite
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "No tests were found"
            -- "${CMAKE_CTEST_COMMAND}" --test-dir "${CMAKE_CURRENT_BINARY_DIR}/empty-suite"
                --no-tests=error
    )
    add_test(NAME headless.default
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected success --contains "ticks=10 step_ns=50000000 elapsed_ns=500000000"
            -- "$<TARGET_FILE:reality_headless>"
    )
    add_test(NAME headless.zero_steps
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected success --contains "ticks=0 step_ns=1000000 elapsed_ns=0"
            -- "$<TARGET_FILE:reality_headless>" --steps 0 --step-ms 1
    )
    add_test(NAME headless.rejects_zero_step
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "step-ms must be positive"
            -- "$<TARGET_FILE:reality_headless>" --step-ms 0
    )
    add_test(NAME headless.rejects_malformed_steps
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "invalid unsigned integer"
            -- "$<TARGET_FILE:reality_headless>" --steps 12junk
    )
    add_test(NAME headless.rejects_overflow
        COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
            --expected failure --contains "simulation time overflow"
            -- "$<TARGET_FILE:reality_headless>" --steps 18446744073709551615
    )
    add_test(NAME documentation.registry
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_docs.py"
    )
    add_test(NAME tooling.tests
        COMMAND "${Python3_EXECUTABLE}" -m unittest discover -s tools/tests -p "test_*.py"
    )
    set_tests_properties(documentation.registry tooling.tests PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    if(REALITY_ENABLE_ASAN)
        add_executable(reality_asan_probe tests/diagnostics/asan_probe.cpp)
        target_link_libraries(reality_asan_probe PRIVATE reality_build_options)
        reality_deploy_diagnostics(reality_asan_probe)
        add_test(NAME diagnostics.clean_fixture
            COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
                --expected success --contains "CLEAN FIXTURE"
                -- "$<TARGET_FILE:reality_asan_probe>" clean
        )
        add_test(NAME diagnostics.detects_heap_overflow
            COMMAND "${Python3_EXECUTABLE}" "${reality_process_check}"
                --expected failure --contains "AddressSanitizer: heap-buffer-overflow"
                -- "$<TARGET_FILE:reality_asan_probe>" heap-overflow
        )
    endif()

    get_property(reality_tests_in_directory DIRECTORY PROPERTY TESTS)
    set_tests_properties(${reality_tests_in_directory} PROPERTIES
        TIMEOUT 20
        ENVIRONMENT "ASAN_OPTIONS=halt_on_error=1;ASAN_VCASAN_DEBUGGING=0"
    )
    # Three sequential processes each have a 15 s subprocess deadline. Keep the
    # parent budget consistent with those deadlines, plus 5 s for Python/reporting.
    set_tests_properties(motion.hardware_gpu_continuation PROPERTIES TIMEOUT 50)
endif()
