cmake_policy(SET CMP0057 NEW)
set(rgx "[A-Za-z0-9.+_-]+.so[0-9.]*")

set(out_dir "${CMAKE_SOURCE_DIR}/redist/lib/")
file(MAKE_DIRECTORY ${out_dir})

execute_process(COMMAND python3 "${CMAKE_CURRENT_LIST_DIR}/get_excludelist.py" OUTPUT_VARIABLE excludelist)

function(copy_libs_for_target target)
    execute_process(COMMAND ldd ${target} OUTPUT_VARIABLE linked_libs)
    string(REGEX MATCHALL "[A-Za-z0-9/]+/x86_64-linux-gnu/${rgx}" reg_match "${linked_libs}")

    foreach (_file IN LISTS reg_match)
        string(REGEX MATCH ${rgx} mtc "${_file}")
        if(${mtc} IN_LIST excludelist)    
            message("Exclude: ${_file}")
        elseif(EXISTS ${_file})
            file(COPY ${_file} DESTINATION ${out_dir} FOLLOW_SYMLINK_CHAIN)
            message("Copy ${_file}")
        else()
            message("Not found: ${_file}")
        endif()
    endforeach()
endfunction()

copy_libs_for_target("${CMAKE_SOURCE_DIR}/redist/RoR")
copy_libs_for_target("${CMAKE_SOURCE_DIR}/redist/lib/Codec_FreeImage.so")
copy_libs_for_target("${CMAKE_SOURCE_DIR}/redist/lib/Plugin_CgProgramManager.so")