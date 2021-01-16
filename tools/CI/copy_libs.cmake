set(out_dir "${CMAKE_SOURCE_DIR}/redist/lib/")
file(MAKE_DIRECTORY ${out_dir})

execute_process(COMMAND ldd ${CMAKE_SOURCE_DIR}/bin/RoR OUTPUT_VARIABLE linked_libs)
string(REGEX MATCHALL "/lib/x86_64-linux-gnu/([A-Za-z0-9.]+.so[0-9.]*)" reg_match "${linked_libs}")

foreach (_file IN LISTS reg_match)
    message("Copy ${_file}")
    file(COPY ${_file} DESTINATION ${out_dir} FOLLOW_SYMLINK_CHAIN)
endforeach()