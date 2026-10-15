# Removes files in TO_DIR that no longer exist in FROM_DIR.
#
# Run as: cmake -DFROM_DIR=<src> -DTO_DIR=<dst> -P PruneStaleFiles.cmake
#
# `fast_copy()` deploys a tree by emitting one copy command per source file, so a file
# deleted from the source tree keeps its stale copy in the build output indefinitely. For
# OGRE scripts that is not merely untidy: material names are global, so an orphaned script
# can win the name over the current definition and produce failures nowhere near the cause.

if (NOT FROM_DIR OR NOT TO_DIR)
    message(FATAL_ERROR "PruneStaleFiles: both FROM_DIR and TO_DIR must be set")
endif ()

if (IS_DIRECTORY "${TO_DIR}")

    file(GLOB_RECURSE deployed_files RELATIVE "${TO_DIR}" "${TO_DIR}/*")
    foreach (file IN LISTS deployed_files)
        if (NOT EXISTS "${FROM_DIR}/${file}")
            message(STATUS "Pruning stale ${file}")
            file(REMOVE "${TO_DIR}/${file}")
        endif ()
    endforeach ()

    # Deepest path last, so reversing collapses nested directories in a single pass.
    file(GLOB_RECURSE deployed_dirs LIST_DIRECTORIES true RELATIVE "${TO_DIR}" "${TO_DIR}/*")
    list(SORT deployed_dirs)
    list(REVERSE deployed_dirs)
    foreach (dir IN LISTS deployed_dirs)
        if (IS_DIRECTORY "${TO_DIR}/${dir}" AND NOT IS_DIRECTORY "${FROM_DIR}/${dir}")
            file(GLOB remaining "${TO_DIR}/${dir}/*")
            if (NOT remaining)
                message(STATUS "Pruning stale directory ${dir}")
                file(REMOVE_RECURSE "${TO_DIR}/${dir}")
            endif ()
        endif ()
    endforeach ()

endif ()
