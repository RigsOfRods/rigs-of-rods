cmake_minimum_required(VERSION 3.8)
project(FreeImage LANGUAGES C CXX)

include(GNUInstallDirs)

find_package(ZLIB REQUIRED)
if(FREEIMAGE_WITH_PNG)
    find_package(PNG REQUIRED)
endif()
if(FREEIMAGE_WITH_JPEG)
    find_package(JPEG REQUIRED)
endif()
if(FREEIMAGE_WITH_OPENJPEG)
    find_package(OpenJPEG REQUIRED CONFIG)
endif()
if(FREEIMAGE_WITH_WEBP)
    find_package(WebP REQUIRED CONFIG)
endif()
if(FREEIMAGE_WITH_JXR)
    find_package(jxrlib REQUIRED CONFIG)
endif()
if(FREEIMAGE_WITH_RAW)
    find_package(libraw REQUIRED CONFIG)
endif()
if(FREEIMAGE_WITH_OPENEXR OR FREEIMAGE_WITH_TIFF)
    find_package(OpenEXR REQUIRED CONFIG)
endif()
if(FREEIMAGE_WITH_TIFF)
    find_package(TIFF REQUIRED)
endif()

# FreeImage

file(GLOB FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/*.cpp ${FREEIMAGE_FOLDER}/Source/FreeImage/*.c)
list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginG3.cpp)
if(NOT FREEIMAGE_WITH_PNG)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginPNG.cpp)
endif()
if(NOT FREEIMAGE_WITH_JPEG)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginJPEG.cpp)
endif()
if(NOT FREEIMAGE_WITH_OPENJPEG)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginJP2.cpp)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginJ2K.cpp)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/J2KHelper.cpp)
endif()
if(NOT FREEIMAGE_WITH_WEBP)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginWebP.cpp)
endif()
if(NOT FREEIMAGE_WITH_JXR)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginJXR.cpp)
endif()
if(NOT FREEIMAGE_WITH_RAW)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginRAW.cpp)
endif()
if(NOT FREEIMAGE_WITH_OPENEXR)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginEXR.cpp)
endif()
if(NOT FREEIMAGE_WITH_TIFF)
    list(REMOVE_ITEM FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImage/PluginTIFF.cpp)
endif()
if(MSVC)
    list(APPEND FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/FreeImage.rc)
endif(MSVC)

file(GLOB FREEIMAGETOOLKIT_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImageToolkit/*.cpp ${FREEIMAGE_FOLDER}/Source/FreeImageToolkit/*.c)
list(REMOVE_ITEM FREEIMAGETOOLKIT_SOURCES ${FREEIMAGE_FOLDER}/Source/FreeImageToolkit/JPEGTransform.cpp)

file(GLOB METADATA_SOURCES ${FREEIMAGE_FOLDER}/Source/Metadata/*.cpp ${FREEIMAGE_FOLDER}/Source/Metadata/*.c)
if(NOT FREEIMAGE_WITH_TIFF)
   list(REMOVE_ITEM METADATA_SOURCES ${FREEIMAGE_FOLDER}/Source/Metadata/XTIFF.cpp)
endif()

add_library(FreeImage ${FREEIMAGE_SOURCES} ${FREEIMAGETOOLKIT_SOURCES} ${METADATA_SOURCES})
target_compile_features(FreeImage PRIVATE cxx_std_11)
set_target_properties(FreeImage PROPERTIES
    OUTPUT_NAME "freeimage"
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
target_include_directories(FreeImage PRIVATE
    ${FREEIMAGE_FOLDER}/Source
    ${FREEIMAGE_FOLDER}/Source/FreeImage
    ${FREEIMAGE_FOLDER}/Source/FreeImageToolkit
    ${FREEIMAGE_FOLDER}/Source/Metadata
)
target_link_libraries(FreeImage PRIVATE ZLIB::ZLIB)
if(FREEIMAGE_WITH_PNG)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_PNG)
    target_link_libraries(FreeImage PRIVATE PNG::PNG)
endif()
if(FREEIMAGE_WITH_JPEG)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_JPEG)
    target_link_libraries(FreeImage PRIVATE JPEG::JPEG)
endif()
if(FREEIMAGE_WITH_OPENJPEG)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_OPENJPEG)
    target_link_libraries(FreeImage PRIVATE openjp2)
endif()
if(FREEIMAGE_WITH_WEBP)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_WEBP)
    target_link_libraries(FreeImage PRIVATE WebP::webp WebP::webpdecoder WebP::libwebpmux WebP::webpdemux)
endif()
if(FREEIMAGE_WITH_JXR)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_JXR)
    target_link_libraries(FreeImage PRIVATE jxrlib::jxrlib)
endif()
if(FREEIMAGE_WITH_RAW)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_RAW)
    target_link_libraries(FreeImage PRIVATE libraw::libraw)
endif()
if(FREEIMAGE_WITH_OPENEXR)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_OPENEXR)
    target_link_libraries(FreeImage PRIVATE openexr::openexr) # TODO: link to exact required components
endif()
if(FREEIMAGE_WITH_TIFF)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_WITH_TIFF)
    target_link_libraries(FreeImage PRIVATE TIFF::TIFF IlmBase::Half)
endif()
if(BUILD_SHARED_LIBS)
    target_compile_definitions(FreeImage PRIVATE FREEIMAGE_EXPORTS)
else()
    target_compile_definitions(FreeImage PUBLIC FREEIMAGE_LIB)
endif()

install(TARGETS FreeImage
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
set(FREEIMAGE_HEADERS ${FREEIMAGE_FOLDER}/Source/FreeImage.h)
install(FILES ${FREEIMAGE_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# FreeImagePlus

set(FREEIMAGEPLUS_HEADERS ${FREEIMAGE_FOLDER}/Wrapper/FreeImagePlus/FreeImagePlus.h)
file(GLOB FREEIMAGEPLUS_SOURCES ${FREEIMAGE_FOLDER}/Wrapper/FreeImagePlus/src/*.cpp ${FREEIMAGE_FOLDER}/Wrapper/FreeImagePlus/src/*.c)
if(MSVC)
    list(APPEND FREEIMAGE_SOURCES ${FREEIMAGE_FOLDER}/Wrapper/FreeImagePlus/FreeImagePlus.rc)
endif(MSVC)
add_library(FreeImagePlus ${FREEIMAGEPLUS_SOURCES})
target_compile_features(FreeImagePlus PUBLIC cxx_std_11)
set_target_properties(FreeImagePlus PROPERTIES
    OUTPUT_NAME "freeimageplus"
    C_VISIBILITY_PRESET hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
)
target_link_libraries(FreeImagePlus PUBLIC FreeImage)
target_include_directories(FreeImagePlus PRIVATE
    ${FREEIMAGE_FOLDER}/Wrapper/FreeImagePlus
    ${FREEIMAGE_FOLDER}/Source
)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(FreeImagePlus PRIVATE FIP_EXPORTS)
endif()

install(TARGETS FreeImagePlus
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(FILES ${FREEIMAGEPLUS_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
