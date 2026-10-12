# Unit tests for range-base-lib (QTest based).
#
# Enabled by configuring the project with -DRANGE_BUILD_TESTS=ON.
# Run with `ctest` from the build directory.

find_package(Qt6 REQUIRED COMPONENTS Test)

if(WIN32 AND CMAKE_VERSION VERSION_LESS 3.22)
    message(FATAL_ERROR "Windows QTest execution requires CMake 3.22 or newer for ENVIRONMENT_MODIFICATION")
endif()

set(CMAKE_AUTOMOC ON)

set(range_base_lib_tests
    tst_hash_sha256
    tst_rvector
    tst_r3vector
    tst_rmatrix
    tst_version
    tst_limit_box
    tst_statistics
    tst_uvector
    tst_interaction_record_v1
)

foreach(test_name IN LISTS range_base_lib_tests)
    qt_add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name}
        PRIVATE
            range-base-lib
            Qt6::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
    if(WIN32)
        set_tests_properties(${test_name} PROPERTIES
            ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
    endif()
endforeach()

# Exercise the Qt portability backend even on the Windows CI host.
qt_add_executable(tst_hash_sha256_qt tst_hash_sha256.cpp ../src/rbl_hash_sha256.cpp)
target_compile_definitions(tst_hash_sha256_qt PRIVATE RANGE_SHA256_TEST_QT_BACKEND)
target_include_directories(tst_hash_sha256_qt PRIVATE ../include)
target_link_libraries(tst_hash_sha256_qt PRIVATE Qt6::Core Qt6::Test)
add_test(NAME tst_hash_sha256_qt COMMAND tst_hash_sha256_qt)
if(WIN32)
    set_tests_properties(tst_hash_sha256_qt PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:Qt6::Core>")
endif()