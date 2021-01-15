
function(extract_pot source_files_list)
    set(FILELIST ${CMAKE_BINARY_DIR}/tmp/xgettext-filelist.txt)
    set(ROR_POT "${CMAKE_SOURCE_DIR}/languages/ror.pot")

    list(JOIN source_files_list "\n" source_files)
    file(WRITE "${FILELIST}" "${source_files}")

    add_custom_target(
            extract_pot
            COMMENT "Extract pot"
            OUTPUT ${ROR_POT}
            COMMAND xgettext -i -F -k_L -k_LC:1c,2 -c -o${ROR_POT} -f ${FILELIST}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )
endfunction()


function(compile_mo)
    file(GLOB_RECURSE potfiles ${CMAKE_SOURCE_DIR}/languages/*/*.po CONFIGURE_DEPENDS)

    foreach (file IN LISTS potfiles)
        get_filename_component(dir ${file} DIRECTORY)
        get_filename_component(filename ${file} NAME_WE)
        get_filename_component(name "${dir}" NAME)
        set(out "${dir}/${filename}.mo")
        add_custom_command(
                COMMENT "Compiling ${name}"
                OUTPUT ${out}
                DEPENDS ${file}
                COMMAND msgfmt -o${out} ${file}
                WORKING_DIRECTORY ${dir}
        )
        list(APPEND MO_FILES ${out})
    endforeach ()

    add_custom_target(
            compile_mo
            DEPENDS ${MO_FILES}
    )
endfunction()