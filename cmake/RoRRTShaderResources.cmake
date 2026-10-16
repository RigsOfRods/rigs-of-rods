# Consume the function library installed with the same OGRE as the linked binary.
# This is deliberately separate from resources/rtshader's sample material scripts.
set(_ror_rtshader_default "")
if(IS_DIRECTORY "${OGRE_MEDIA_DIR}/RTShaderLib/GLSL")
    set(_ror_rtshader_default "${OGRE_MEDIA_DIR}/RTShaderLib/GLSL")
endif()
set(ROR_RTSHADER_GLSL_DIR "${_ror_rtshader_default}" CACHE PATH
    "GLSL function library from the OGRE installation used to build RoR")

if(ROR_RTSHADER_GLSL_DIR OR ROR_MACOS_GL3_PROFILE)
    foreach(_shader FFPLib_Common.glsl FFPLib_Transform.glsl FFPLib_Texturing.glsl
                    FFPLib_Fog.glsl FFPLib_AlphaTest.glsl SGXLib_PerPixelLighting.glsl)
        if(NOT EXISTS "${ROR_RTSHADER_GLSL_DIR}/${_shader}")
            message(FATAL_ERROR "ROR_RTSHADER_GLSL_DIR must contain the matching OGRE RTShaderLib/GLSL directory; missing ${_shader} in '${ROR_RTSHADER_GLSL_DIR}'")
        endif()
    endforeach()

    # Copy the complete, bounded GLSL directory, never the enclosing sample media.
    # Run on resource builds even when no source changes require an executable relink.
    add_custom_target(ror_rtshader_resources ALL
        COMMAND "${CMAKE_COMMAND}" -E remove_directory "${RUNTIME_OUTPUT_DIRECTORY}/resources/rtshader-core"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${RUNTIME_OUTPUT_DIRECTORY}/resources/rtshader-core"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${ROR_RTSHADER_GLSL_DIR}" "${RUNTIME_OUTPUT_DIRECTORY}/resources/rtshader-core"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_SOURCE_DIR}/source/main/gui/imgui/LICENSE-OGRE.txt"
            "${RUNTIME_OUTPUT_DIRECTORY}/resources/rtshader-core/LICENSE-OGRE.txt"
        VERBATIM)
    add_dependencies(${BINNAME} ror_rtshader_resources)
endif()
