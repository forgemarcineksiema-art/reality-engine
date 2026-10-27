cmake_minimum_required(VERSION 3.25)
project(GenesisObstructionBaseline LANGUAGES CXX)
set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
add_executable(obstruction_baseline "${ROOT}/tests/traversal_audit_tests.cpp"
    traversal_session.cpp)
target_include_directories(obstruction_baseline PRIVATE "${ROOT}/src")
target_compile_features(obstruction_baseline PRIVATE cxx_std_20)
target_compile_options(obstruction_baseline PRIVATE /W4 /WX /permissive-)
target_link_libraries(obstruction_baseline PRIVATE "${ROOT}/build/msvc/Debug/reality_lab_model.lib"
                                                 "${ROOT}/build/msvc/Debug/reality_core.lib")
