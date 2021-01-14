set(base_dir "${CMAKE_BINARY_DIR}/../../")
get_filename_component(base_dir ${base_dir} ABSOLUTE)

file(GLOB_RECURSE cpp_files "${base_dir}/source/*.cpp" RELATIVE "${base_dir}" LIST_DIRECTORIES FALSE)

set(ror_pot "${base_dir}/languages/ror.pot")
set(tmp_pot "${CMAKE_BINARY_DIR}/TMP.pot")

file(WRITE ${tmp_pot} "")

foreach (_file IN LISTS cpp_files)
    message("Extracting strings from ${_file}")
    file(RELATIVE_PATH relpath "${base_dir}" "${_file}")
    execute_process(COMMAND xgettext -o${tmp_pot} -k_L -k_LC:1c,2 -c -j ${relpath}
        WORKING_DIRECTORY "${base_dir}")
endforeach ()

message("Cleaning up")
execute_process(COMMAND msguniq -i -F -o${ror_pot} ${tmp_pot}
                WORKING_DIRECTORY ${base_dir})