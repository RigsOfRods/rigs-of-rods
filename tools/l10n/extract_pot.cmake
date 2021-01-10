set(base_dir "${CMAKE_BINARY_DIR}/../../")
get_filename_component(base_dir ${base_dir} ABSOLUTE)

file(GLOB_RECURSE cpp_files "${base_dir}/source/*.cpp" RELATIVE "${base_dir}" LIST_DIRECTORIES FALSE)

set(ror_pot "${base_dir}/languages/ror.pot")

foreach (_file IN LISTS cpp_files)
    message("Extracting strings from ${_file}")
    file(RELATIVE_PATH relpath "${base_dir}" "${_file}")
    execute_process(COMMAND xgettext -o${ror_pot} -k_L -k_LC:1c,2 -c -j ${relpath} 
        WORKING_DIRECTORY "${base_dir}")
endforeach ()

message("Cleaning up")
execute_process(COMMAND msguniq -i -s -o${ror_pot} ${ror_pot}
                WORKING_DIRECTORY ${base_dir})