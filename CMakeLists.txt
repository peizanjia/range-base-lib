# Create a static library
qt_add_library(range-base-lib
    STATIC
        src/rbl_hash_sha256.cpp
        src/rbl_application_state.cpp
        src/rbl_argument_option.cpp
        src/rbl_arguments_parser.cpp
        src/rbl_book.cpp
        src/rbl_bvector.cpp
        src/rbl_error.cpp
        src/rbl_file_tools.cpp
        src/rbl_imatrix.cpp
        src/rbl_interaction_record_v1.cpp
        src/rbl_ivector.cpp
        src/rbl_job.cpp
        src/rbl_job_manager.cpp
        src/rbl_job_settings.cpp
        src/rbl_limit_box.cpp
        src/rbl_locker.cpp
        src/rbl_logger.cpp
        src/rbl_message.cpp
        src/rbl_plane.cpp
        src/rbl_progress.cpp
        src/rbl_r3vector.cpp
        src/rbl_rmatrix.cpp
        src/rbl_rvector.cpp
        src/rbl_statistics.cpp
        src/rbl_stop_watch.cpp
        src/rbl_tool_action.cpp
        src/rbl_tool_input.cpp
        src/rbl_tool_task.cpp
        src/rbl_utils.cpp
        src/rbl_uvector.cpp
        src/rbl_value_table.cpp
        src/rbl_value_vector.cpp
        src/rbl_version.cpp
        src/rbl_local_direction.cpp
        include/rbl_hash_sha256.h
        include/rbl_application_state.h
        include/rbl_argument_option.h
        include/rbl_arguments_parser.h
        include/rbl_book.h
        include/rbl_bvector.h
        include/rbl_distance_vector.h
        include/rbl_error.h
        include/rbl_file_tools.h
        include/rbl_imatrix.h
        include/rbl_interaction_record_v1.h
        include/rbl_ivector.h
        include/rbl_job.h
        include/rbl_job_manager.h
        include/rbl_job_settings.h
        include/rbl_limit_box.h
        include/rbl_local_direction.h
        include/rbl_locker.h
        include/rbl_logger.h
        include/rbl_message.h
        include/rbl_plane.h
        include/rbl_progress.h
        include/rbl_r3vector.h
        include/rbl_rmatrix.h
        include/rbl_rvector.h
        include/rbl_statistics.h
        include/rbl_stop_watch.h
        include/rbl_tool_action.h
        include/rbl_tool_input.h
        include/rbl_tool_task.h
        include/rbl_utils.h
        include/rbl_uvector.h
        include/rbl_value_table.h
        include/rbl_value_vector.h
        include/rbl_version.h
)

# Include directories
target_include_directories(range-base-lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(range-base-lib
    PUBLIC
        Qt6::Core
        common_defines
)

# Unit tests (opt-in via -DRANGE_BUILD_TESTS=ON)
if(RANGE_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# Windows built-in CNG SHA-256; no redistributed crypto library.
if(WIN32)
    target_link_libraries(range-base-lib PRIVATE bcrypt)
endif()
