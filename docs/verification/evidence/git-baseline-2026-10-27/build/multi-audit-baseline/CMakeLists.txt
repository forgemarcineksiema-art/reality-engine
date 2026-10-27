cmake_minimum_required(VERSION 3.25)
project(GenesisAuditBaseline LANGUAGES CXX)
set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_executable(audit_baseline "${ROOT}/tests/traversal_audit_tests.cpp"
    traversal_session.cpp traversal_course.cpp platform_motion.cpp)
target_include_directories(audit_baseline PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${ROOT}/src")
target_compile_features(audit_baseline PRIVATE cxx_std_20)
target_compile_options(audit_baseline PRIVATE /W4 /WX /permissive-)
target_link_libraries(audit_baseline PRIVATE "${ROOT}/build/msvc/Debug/reality_lab_model.lib"
                                          "${ROOT}/build/msvc/Debug/reality_core.lib")
