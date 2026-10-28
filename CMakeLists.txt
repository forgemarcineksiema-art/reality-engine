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
    src/core/runtime_metrics.cpp
    src/simulation/work_world.cpp
    src/simulation/material_transfer.cpp
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

add_library(reality_learning_core STATIC src/research/discovery_world.cpp src/research/discovery_agent.cpp
    src/research/hypothesis_program.cpp src/research/contrast_agent.cpp)
target_link_libraries(reality_learning_core PUBLIC reality_core)
add_library(reality_lab_model STATIC src/lab/traversal_practice_codec.cpp src/lab/traversal_practice.cpp src/lab/lab_session.cpp src/graphics/lab_scene.cpp
    src/physics/kinematic.cpp src/navigation/grid_navigation.cpp src/lab/motion_session.cpp
    src/physics/spatial_geometry.cpp src/navigation/surface_navigation.cpp src/physics/locomotion.cpp
    src/physics/platform_motion.cpp src/lab/traversal_session.cpp src/lab/traversal_course.cpp
    src/lab/traversal_codec.cpp src/lab/traversal_diagnostics.cpp src/lab/traversal_needs.cpp
    src/lab/traversal_clearance.cpp src/lab/traversal_delivery.cpp src/lab/traversal_knowledge.cpp src/lab/traversal_law.cpp src/lab/traversal_passages.cpp src/graphics/traversal_scene.cpp)
target_link_libraries(reality_lab_model PUBLIC reality_core reality_learning_core)

add_library(reality_discovery_model STATIC src/research/discovery_session.cpp src/research/discovery_codec.cpp src/research/discovery_protocol.cpp)
target_link_libraries(reality_discovery_model PUBLIC reality_lab_model)
target_link_libraries(reality_discovery_model PRIVATE psapi)
add_executable(reality_discovery src/headless/discovery_main.cpp)
target_link_libraries(reality_discovery PRIVATE reality_discovery_model)
reality_deploy_diagnostics(reality_discovery)
add_library(reality_hypothesis_model STATIC src/research/hypothesis_agent.cpp
    src/research/hypothesis_session.cpp src/research/hypothesis_codec.cpp src/research/hypothesis_protocol.cpp)
target_link_libraries(reality_hypothesis_model PUBLIC reality_discovery_model)
add_executable(reality_hypothesis src/headless/hypothesis_main.cpp)
target_link_libraries(reality_hypothesis PRIVATE reality_hypothesis_model)
reality_deploy_diagnostics(reality_hypothesis)

add_library(reality_graphics STATIC src/graphics/d3d11_renderer.cpp)
add_library(reality_contrast_model STATIC src/research/contrast_session.cpp)
target_link_libraries(reality_contrast_model PUBLIC reality_hypothesis_model)
add_executable(reality_contrast src/headless/contrast_main.cpp)
add_custom_target(contrast_identity
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/write_contrast_identity.py"
        "${CMAKE_CURRENT_BINARY_DIR}/contrast_identity.hpp"
    BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/contrast_identity.hpp"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
add_dependencies(reality_contrast contrast_identity)
target_include_directories(reality_contrast PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_definitions(reality_contrast PRIVATE
    REALITY_CONTRAST_CONFIG="$<CONFIG>" REALITY_CONTRAST_ASAN=$<BOOL:${REALITY_ENABLE_ASAN}>)
target_link_libraries(reality_contrast PRIVATE reality_contrast_model)
reality_deploy_diagnostics(reality_contrast)
target_link_libraries(reality_graphics PUBLIC reality_lab_model d3d11 dxgi d3dcompiler windowscodecs ole32)
target_compile_definitions(reality_graphics PUBLIC WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE)

add_executable(reality_lab WIN32 src/platform/lab_main.cpp)
target_link_libraries(reality_lab PRIVATE reality_graphics user32 gdi32 shell32)
reality_deploy_diagnostics(reality_lab)
add_executable(reality_traversal WIN32 src/platform/traversal_main.cpp)
target_link_libraries(reality_traversal PRIVATE reality_graphics user32 gdi32 shell32 psapi)
reality_deploy_diagnostics(reality_traversal)

set(reality_cpp_files
    src/lab/traversal_practice_codec.cpp
    src/lab/traversal_practice.hpp
    src/lab/traversal_practice.cpp
    tests/practice_tests.cpp
    src/headless/contrast_main.cpp
    src/research/contrast_agent.hpp
    src/research/contrast_agent.cpp
    src/research/contrast_session.hpp
    src/research/contrast_session.cpp
    tests/contrast_tests.cpp
    src/research/hypothesis_protocol.hpp
    src/research/hypothesis_protocol.cpp
    src/headless/hypothesis_main.cpp
    src/research/hypothesis_session.hpp
    src/research/hypothesis_session.cpp
    src/research/hypothesis_codec.cpp
    src/research/hypothesis_agent.hpp
    src/research/hypothesis_agent.cpp
    src/research/hypothesis_program.hpp
    src/research/hypothesis_program.cpp
    tests/hypothesis_tests.cpp
    src/research/discovery_world.hpp
    src/research/discovery_world.cpp
    src/research/discovery_agent.hpp
    src/research/discovery_agent.cpp
    src/research/discovery_random.hpp
    src/research/discovery_session.hpp
    src/research/discovery_session.cpp
    src/research/discovery_codec.cpp
    src/research/discovery_protocol.hpp
    src/research/discovery_protocol.cpp
    src/headless/discovery_main.cpp
    tests/discovery_tests.cpp
    src/core/simulation_clock.hpp
    src/core/simulation_clock.cpp
    src/core/runtime_metrics.hpp
    src/core/runtime_metrics.cpp
    src/lab/traversal_diagnostics.hpp
    src/lab/traversal_diagnostics.cpp
    tests/traversal_diagnostics_tests.cpp
    tests/traversal_needs_tests.cpp
    src/lab/traversal_needs.cpp
    src/lab/traversal_clearance.cpp
    src/lab/traversal_delivery.cpp
    src/lab/traversal_knowledge.cpp
    src/lab/traversal_law.cpp
    tests/law_tests.cpp
    tests/community_tests.cpp
    src/lab/traversal_passages.cpp
    tests/passage_tests.cpp
    tests/knowledge_tests.cpp
    tests/delivery_tests.cpp
    tests/clearance_tests.cpp
    src/headless/main.cpp
    src/core/entity_registry.hpp
    src/core/entity_registry.cpp
    src/simulation/work_world.hpp
    src/core/bounded_records.hpp
    src/simulation/material_transfer.cpp
    tests/transfer_tests.cpp
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
    add_executable(reality_practice_tests tests/practice_tests.cpp)
    target_link_libraries(reality_practice_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_practice_tests)
    foreach(case IN ITEMS calibration energy concurrent no_parts out_of_reach protected queued_input persistence corruption legacy horizon autonomy teaching alternate blocked_contact safe_queue needs manual_override community)
        add_test(NAME practice.${case} COMMAND reality_practice_tests ${case})
    endforeach()
    add_test(NAME practice.fresh_process COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/practice_integration.py" "$<TARGET_FILE:reality_practice_tests>")
    add_test(NAME practice.native COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_practice_native.py" --exe "$<TARGET_FILE:reality_traversal>")
    add_test(NAME contrast.native_case COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/contrast_case_integration.py" "$<TARGET_FILE:reality_contrast>")
    add_test(NAME contrast.fresh_process COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/contrast_integration.py" "$<TARGET_FILE:reality_contrast>")
    add_executable(reality_contrast_tests tests/contrast_tests.cpp)
    target_link_libraries(reality_contrast_tests PRIVATE reality_contrast_model)
    reality_deploy_diagnostics(reality_contrast_tests)
    foreach(case IN ITEMS agent physical work pending depleted boundary)
        add_test(NAME contrast.${case} COMMAND reality_contrast_tests ${case})
    endforeach()
    add_executable(reality_hypothesis_tests tests/hypothesis_tests.cpp)
    target_link_libraries(reality_hypothesis_tests PRIVATE reality_hypothesis_model)
    reality_deploy_diagnostics(reality_hypothesis_tests)
    foreach(case IN ITEMS program apparatus learning limits session continuation capacity interrupted)
        add_test(NAME hypothesis.${case} COMMAND reality_hypothesis_tests ${case})
    endforeach()
    add_test(NAME hypothesis.native_continuation
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/hypothesis_integration.py"
                --exe "$<TARGET_FILE:reality_hypothesis>")
    add_test(NAME hypothesis.native_case
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/hypothesis_integration.py"
                --exe "$<TARGET_FILE:reality_hypothesis>" --mode case)
    add_executable(reality_discovery_tests tests/discovery_tests.cpp)
    target_link_libraries(reality_discovery_tests PRIVATE reality_discovery_model)
    reality_deploy_diagnostics(reality_discovery_tests)
    foreach(case IN ITEMS world replay refusals world_restore learning minimax composition frozen outside access contact isolation continuation corruption blocked_share budgets pending_observation)
        add_test(NAME discovery.${case} COMMAND reality_discovery_tests ${case})
    endforeach()
    foreach(mode IN ITEMS case continuation)
        add_test(NAME discovery.native_${mode}
            COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/discovery_integration.py"
                    --exe "$<TARGET_FILE:reality_discovery>" --mode ${mode})
    endforeach()
    add_executable(reality_community_tests tests/community_tests.cpp)
    target_link_libraries(reality_community_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_community_tests)
    foreach(case IN ITEMS scene calm disturbed law continuation adaptive_calm adaptive_disturbed fixed_calm fixed_disturbed)
        add_test(NAME community.${case} COMMAND reality_community_tests ${case})
    endforeach()
    add_test(NAME community.native_diagnostics
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_community_native.py"
                --exe "$<TARGET_FILE:reality_traversal>")
    add_executable(reality_law_tests tests/law_tests.cpp)
    target_link_libraries(reality_law_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_law_tests)
    foreach(case IN ITEMS unseen isolated relay restitution repeated_return owner geometry authorized remote_return bounded_auto multiple corruption legacy empty_corruption settlement_chronology unseen_return multi_relay safe_action limits legacy_sections)
        add_test(NAME law.${case} COMMAND reality_law_tests ${case})
    endforeach()
    add_test(NAME law.native_diagnostics
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_law_native.py"
                --exe "$<TARGET_FILE:reality_traversal>")
    add_executable(reality_passage_tests tests/passage_tests.cpp)
    target_link_libraries(reality_passage_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_passage_tests)
    foreach(case IN ITEMS discovery witness repair occupied step_refusal local_manual community persistence corruption observation_ring wear_and_queue manual_barrier no_path_scout protected_ride protected_step long_detour protected_boarding protected_exiting opening_report queue_cancel repair_resume unlabelled_bank action_report clearance_refusal delivery_cost open_observation)
        add_test(NAME passage.${case} COMMAND reality_passage_tests ${case})
    endforeach()
    add_executable(reality_knowledge_tests tests/knowledge_tests.cpp)
    target_link_libraries(reality_knowledge_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_knowledge_tests)
    foreach(case IN ITEMS no_global witness delivery persistence stale contact corruption relay scout safe_action bounded_auto autonomous_delivery safe_step safe_boarding safe_riding safe_exiting safe_boarding_expired legacy4 safe_clearance)
        add_test(NAME knowledge.${case} COMMAND reality_knowledge_tests ${case})
    endforeach()
    add_executable(reality_delivery_tests tests/delivery_tests.cpp)
    target_link_libraries(reality_delivery_tests PRIVATE reality_lab_model)
    target_compile_definitions(reality_delivery_tests PRIVATE
        REALITY_DELIVERY_CHECKPOINT="${CMAKE_CURRENT_BINARY_DIR}/delivery-actual-checkpoint.save")
    reality_deploy_diagnostics(reality_delivery_tests)
    add_test(NAME delivery.actual_outbound COMMAND reality_delivery_tests actual_outbound)
    add_test(NAME delivery.actual_completion COMMAND reality_delivery_tests actual_completion)
    set_tests_properties(delivery.actual_outbound PROPERTIES FIXTURES_SETUP delivery_actual)
    set_tests_properties(delivery.actual_completion PROPERTIES FIXTURES_REQUIRED delivery_actual)
    foreach(case IN ITEMS completed local_and_cancel persistence needs manual no_recycling wrong_station corruption meals blocked cancel_step return_cancelled return_interaction return_fallback return_hunger return_detour return_step return_manual boat aliased_lots wall wall_approach legacy_wall_need station_kind)
        add_test(NAME delivery.${case} COMMAND reality_delivery_tests ${case})
    endforeach()
    add_executable(reality_transfer_tests tests/transfer_tests.cpp)
    target_link_libraries(reality_transfer_tests PRIVATE reality_core)
    target_compile_definitions(reality_transfer_tests PRIVATE
        REALITY_TRANSFER_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    reality_deploy_diagnostics(reality_transfer_tests)
    foreach(case IN ITEMS basic reserved replay refusals large_cycle removed boundaries allocation corruption legacy)
        add_test(NAME transfer.${case} COMMAND reality_transfer_tests ${case})
    endforeach()
    add_executable(reality_clearance_tests tests/clearance_tests.cpp)
    target_link_libraries(reality_clearance_tests PRIVATE reality_lab_model)
    target_compile_definitions(reality_clearance_tests PRIVATE
        REALITY_CLEARANCE_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/docs/verification/evidence/retained-world-stall-6938/world.save")
    reality_deploy_diagnostics(reality_clearance_tests)
    foreach(case IN ITEMS step_blockade controlled retained persistence no_space busy manual_override same_intent_override invalid_restore reserved_targets expiry intent_preserved stale_step_restore)
        add_test(NAME clearance.${case} COMMAND reality_clearance_tests ${case})
    endforeach()
    add_executable(reality_traversal_needs_tests tests/traversal_needs_tests.cpp)
    target_link_libraries(reality_traversal_needs_tests PRIVATE reality_lab_model)
    target_compile_definitions(reality_traversal_needs_tests PRIVATE
        REALITY_NEEDS_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/traversal-v1-needs.save")
    reality_deploy_diagnostics(reality_traversal_needs_tests)
    foreach(case IN ITEMS interruption exhaustion safe_step safe_transport running_action manual_override unavailable_need persistence finish_expiry legacy_fixture safe_queue)
        add_test(NAME needs.${case} COMMAND reality_traversal_needs_tests ${case})
    endforeach()
    add_executable(reality_traversal_diagnostics_tests tests/traversal_diagnostics_tests.cpp)
    target_link_libraries(reality_traversal_diagnostics_tests PRIVATE reality_lab_model)
    reality_deploy_diagnostics(reality_traversal_diagnostics_tests)
    foreach(case IN ITEMS progress waiting reset snapshot timings invalid_sample plan_churn independent_alarms service_progress batch_transaction live_view)
        add_test(NAME traversal.diagnostics_${case} COMMAND reality_traversal_diagnostics_tests ${case})
    endforeach()
    add_test(NAME traversal.native_diagnostics
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_traversal_diagnostics.py"
                --exe "$<TARGET_FILE:reality_traversal>")
    add_test(NAME delivery.native_diagnostics
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_traversal_diagnostics.py"
                --exe "$<TARGET_FILE:reality_traversal>" --delivery-only)
    add_test(NAME passage.native_diagnostics
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/test_crossing_native.py"
                --exe "$<TARGET_FILE:reality_traversal>")
    add_executable(reality_traversal_audit_tests tests/traversal_audit_tests.cpp)
    target_link_libraries(reality_traversal_audit_tests PRIVATE reality_lab_model)
    target_compile_definitions(reality_traversal_audit_tests PRIVATE
        REALITY_STALL_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/docs/verification/evidence/retained-world-stall-6938/world.save")
    reality_deploy_diagnostics(reality_traversal_audit_tests)
    foreach(case IN ITEMS narrow_bridge_opposite ferry_exchange rapid_retarget obstacle_braking
            slope_maneuvers rotating_support crowd_passing occupied_goal_recovery snapshot_corruption long_course partition
            collinear_rest moving_passengers command_isolation support_contract moving_contact
            wall_neighbour_recovery blocked_route_settling occupied_queue_parking
            approach_blocked_recovery approach_recovery_detour retained_ramp_recovery
            approach_alternative_retry
            approach_existing_retry
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
