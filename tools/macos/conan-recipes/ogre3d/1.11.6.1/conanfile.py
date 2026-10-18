from conan import ConanFile
from conan.tools.files import get, collect_libs, rmdir, replace_in_file, apply_conandata_patches, export_conandata_patches
from conan.tools.cmake import CMakeToolchain, CMake, CMakeDeps, cmake_layout
from conan.tools.system.package_manager import Apt
import os

class OGREConan(ConanFile):
    name = "ogre3d"
    license = "MIT"
    url = "https://github.com/AnotherFoxGuy/conan-OGRE"
    description = "scene-oriented, flexible 3D engine written in C++"
    settings = "os", "compiler", "build_type", "arch"
    
    options = {
        "resourcemanager_strict": ["off", "pedantic", "strict"],
        "nodeless_positioning": [True, False],
        "profiling": [True, False],
        "profiling_remotery": [True, False],
    }

    default_options = {
        "resourcemanager_strict": "off",
        "nodeless_positioning": True,
        "profiling": False,
        "profiling_remotery": False,
    }

    def export_sources(self):
        export_conandata_patches(self)

    def layout(self):
        cmake_layout(self)

    def requirements(self):
        # self.requires("zlib/[~1]")
        self.requires("zziplib/[~0]")
        self.requires("freetype/[~2]")
        self.requires("freeimage/3.18.0@anotherfoxguy/stable")
        self.requires("pugixml/[~1]")
        if self.settings.os == "Windows":
            self.requires("directx-sdk/9.0@anotherfoxguy/stable")

        self.requires("libpng/1.6.40", override=True)
        self.requires("libwebp/1.3.2", override=True)
        self.requires("zlib/1.3", force=True)

    def system_requirements(self):
        Apt(self).install([
                "libx11-dev",
                "libxaw7-dev",
                "libxrandr-dev",
                "libgles2-mesa-dev",
                "libglu1-mesa-dev",
                "libvulkan-dev",
                "glslang-dev"
        ], check=True)

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)
        replace_in_file(self, os.path.join(self.source_folder, "CMakeLists.txt"),
                        "set(CMAKE_OSX_SYSROOT macosx)",
                        "# CMake sysroot supplied by Conan toolchain")
        replace_in_file(self, os.path.join(self.source_folder, "OgreMain/include/OgrePlatformInformation.h"),
                        "#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE\n#   define OGRE_CPU OGRE_CPU_X86",
                        "#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE && (defined(__i386__) || defined(__x86_64__))\n#   define OGRE_CPU OGRE_CPU_X86")

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["OGRE_BUILD_COMPONENT_BITES"] = "ON"
        tc.variables["OGRE_BUILD_COMPONENT_CSHARP"] = "OFF"
        tc.variables["OGRE_BUILD_COMPONENT_JAVA"] = "OFF"
        tc.variables["OGRE_BUILD_COMPONENT_OVERLAY_IMGUI"] = "ON"
        tc.variables["OGRE_BUILD_COMPONENT_PYTHON"] = "OFF"
        tc.variables["OGRE_BUILD_DEPENDENCIES"] = "OFF"
        tc.variables["OGRE_BUILD_PLUGIN_STBI"] = "ON"
        tc.variables["OGRE_BUILD_PLUGIN_EXRCODEC"] = "OFF"
        tc.variables["OGRE_BUILD_RENDERSYSTEM_D3D11"] = "ON"
        tc.variables["OGRE_BUILD_RENDERSYSTEM_D3D9"] = "ON"
        tc.variables["OGRE_BUILD_RENDERSYSTEM_GL3PLUS"] = "OFF"
        tc.variables["OGRE_BUILD_SAMPLES"] = "OFF"
        tc.variables["OGRE_INSTALL_SAMPLES"] = "OFF"
        tc.variables["OGRE_RESOURCEMANAGER_STRICT"] = 0
        tc.variables["OGRE_NODELESS_POSITIONING"] = self.options.nodeless_positioning
        tc.variables["OGRE_PROFILING"] = self.options.profiling
        tc.variables["OGRE_PROFILING_REMOTERY"] = self.options.profiling_remotery

        if self.options.resourcemanager_strict == "off":
            tc.variables["OGRE_RESOURCEMANAGER_STRICT"] = 0
        elif self.options.resourcemanager_strict == "pedantic":
            tc.variables["OGRE_RESOURCEMANAGER_STRICT"] = 1
        else:
            tc.variables["OGRE_RESOURCEMANAGER_STRICT"] = 2

        if self.settings.os == "Windows":
            tc.variables["CMAKE_CXX_FLAGS"] = "-D_OGRE_FILESYSTEM_ARCHIVE_UNICODE"

        tc.cache_variables["CMAKE_POLICY_VERSION_MINIMUM"] = "3.5"
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def _patch_sources(self):
        apply_conandata_patches(self)
        replace_in_file(self,
            os.path.join(self.source_folder, "RenderSystems/GL/CMakeLists.txt"),
            '-framework OpenGL -framework AGL',
            '-framework OpenGL')
        replace_in_file(self,
            os.path.join(self.source_folder, "OgreMain/src/OgrePlatformInformation.cpp"),
            "        uint features = 0;\n#if defined(__ARM_NEON__)",
            "        uint features = 0;\n#if defined(__aarch64__) && OGRE_PLATFORM == OGRE_PLATFORM_APPLE\n        return 0;\n#endif\n#if defined(__ARM_NEON__)")
        replace_in_file(self,
            os.path.join(self.source_folder, "CMake/Dependencies.cmake"),
            "find_package(DirectX)",
            "find_package(DirectX9)",
        )
        replace_in_file(self,
            os.path.join(self.source_folder, "CMake/Packages/FindDirectX11.cmake"),
            'find_path(DirectX11_INCLUDE_DIR NAMES d3d11.h HINTS "',
            'find_path(DirectX11_INCLUDE_DIR NO_CMAKE_PATH NO_CMAKE_ENVIRONMENT_PATH NAMES d3d11.h HINTS "',
        )

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        cmake = CMake(self)
        cmake.install()
        rmdir(self, os.path.join(self.package_folder, "CMake"))
        rmdir(self, os.path.join(self.package_folder, "Docs"))

    def package_info(self):
        self.cpp_info.set_property("cmake_module_file_name", "OGRE")
        self.cpp_info.set_property("cmake_module_target_name", "OGRE::OGRE")
        self.cpp_info.set_property("cmake_file_name", "OGRE")
        self.cpp_info.set_property("cmake_target_name", "OGRE::OGRE")
        self.cpp_info.includedirs = [
            "include",
            "include/OGRE",
            "include/OGRE/Bites",
            "include/OGRE/MeshLodGenerator",
            "include/OGRE/Overlay",
            "include/OGRE/Paging",
            "include/OGRE/Plugins",
            "include/OGRE/Property",
            "include/OGRE/RenderSystems",
            "include/OGRE/RTShaderSystem",
            "include/OGRE/Terrain",
            "include/OGRE/Threading",
            "include/OGRE/Volume",
        ]
        self.cpp_info.libs = collect_libs(self)
