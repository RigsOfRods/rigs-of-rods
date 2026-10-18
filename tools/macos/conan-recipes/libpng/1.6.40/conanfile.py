from conan import ConanFile
from conan.errors import ConanInvalidConfiguration
from conan.tools.apple import is_apple_os
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import replace_in_file, apply_conandata_patches, copy, export_conandata_patches, get, rm, rmdir
from conan.tools.microsoft import is_msvc
from conan.tools.scm import Version
import os

required_conan_version = ">=1.53.0"


class LibpngConan(ConanFile):
    name = "libpng"
    description = "libpng is the official PNG file format reference library."
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "http://www.libpng.org"
    license = "libpng-2.0"
    topics = ("png", "graphics", "image")
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "neon": [True, "check", False],
        "msa": [True, False],
        "sse": [True, False],
        "vsx": [True, False],
        "api_prefix": ["ANY"],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "neon": True,
        "msa": True,
        "sse": True,
        "vsx": True,
        "api_prefix": "",
    }

    @property
    def _is_clang_cl(self):
        return self.settings.os == "Windows" and self.settings.compiler == "clang" and \
               self.settings.compiler.get_safe("runtime")

    @property
    def _has_neon_support(self):
        return "arm" in self.settings.arch

    @property
    def _has_msa_support(self):
        return "mips" in self.settings.arch

    @property
    def _has_sse_support(self):
        return self.settings.arch in ["x86", "x86_64"]

    @property
    def _has_vsx_support(self):
        return "ppc" in self.settings.arch

    @property
    def _neon_msa_sse_vsx_mapping(self):
        return {
            "True": "on",
            "False": "off",
            "check": "check",
        }

    def export_sources(self):
        export_conandata_patches(self)
        copy(self, "conan_cmake_project_include.cmake", self.recipe_folder, os.path.join(self.export_sources_folder, "src"))

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC
        if not self._has_neon_support:
            del self.options.neon
        if not self._has_msa_support:
            del self.options.msa
        if not self._has_sse_support:
            del self.options.sse
        if not self._has_vsx_support:
            del self.options.vsx

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.settings.rm_safe("compiler.libcxx")
        self.settings.rm_safe("compiler.cppstd")

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires("zlib/[>=1.2.11 <2]")

    def validate(self):
        if Version(self.version) < "1.6" and self.settings.arch == "armv8" and is_apple_os(self):
            raise ConanInvalidConfiguration(f"{self.ref} currently does not building for {self.settings.os} {self.settings.arch}. Contributions are welcomed")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["PNG_TESTS"] = False
        tc.cache_variables["PNG_SHARED"] = self.options.shared
        tc.cache_variables["PNG_STATIC"] = not self.options.shared
        tc.cache_variables["PNG_DEBUG"] = self.settings.build_type == "Debug"
        tc.cache_variables["PNG_PREFIX"] = self.options.api_prefix
        if Version(self.version) < "1.6.38":
            tc.cache_variables["CMAKE_PROJECT_libpng_INCLUDE"] = os.path.join(self.source_folder, "conan_cmake_project_include.cmake")
        if self._has_neon_support:
            tc.variables["PNG_ARM_NEON"] = self._neon_msa_sse_vsx_mapping[str(self.options.neon)]
        if self._has_msa_support:
            tc.variables["PNG_MIPS_MSA"] = self._neon_msa_sse_vsx_mapping[str(self.options.msa)]
        if self._has_sse_support:
            tc.variables["PNG_INTEL_SSE"] = self._neon_msa_sse_vsx_mapping[str(self.options.sse)]
        if self._has_vsx_support:
            tc.variables["PNG_POWERPC_VSX"] = self._neon_msa_sse_vsx_mapping[str(self.options.vsx)]
        if Version(self.version) >= "1.6.41":
            tc.variables["PNG_FRAMEWORK"] = False  # changed from False to True by default in PNG 1.6.41
            tc.variables["PNG_TOOLS"] = False
        elif Version(self.version) >= "1.6.38":
            tc.variables["PNG_EXECUTABLES"] = False

        tc.cache_variables["CMAKE_MACOSX_BUNDLE"] = False
        tc.generate()
        tc = CMakeDeps(self)
        tc.generate()

    def _patch_sources(self):
        apply_conandata_patches(self)
        replace_in_file(self, os.path.join(self.source_folder, "pngpriv.h"),
                        "#      include <fp.h>",
                        "#      include <math.h>")

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        copy(self, "LICENSE", src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()
        if self.options.shared:
            rm(self, "*[!.dll]", os.path.join(self.package_folder, "bin"))
        else:
            rmdir(self, os.path.join(self.package_folder, "bin"))
        rmdir(self, os.path.join(self.package_folder, "lib", "libpng"))
        rmdir(self, os.path.join(self.package_folder, "lib", "pkgconfig"))
        rmdir(self, os.path.join(self.package_folder, "share"))
        rm(self, "*.cmake", os.path.join(self.package_folder, "lib", "cmake", "PNG"))

    def package_info(self):
        major_min_version = f"{Version(self.version).major}{Version(self.version).minor}"

        self.cpp_info.set_property("cmake_find_mode", "both")
        self.cpp_info.set_property("cmake_file_name", "PNG")
        self.cpp_info.set_property("cmake_target_name", "PNG::PNG")
        self.cpp_info.set_property("pkg_config_name", "libpng")
        self.cpp_info.set_property("pkg_config_aliases", [f"libpng{major_min_version}"])

        prefix = "lib" if (is_msvc(self) or self._is_clang_cl) else ""
        suffix = major_min_version if self.settings.os == "Windows" else ""
        if is_msvc(self) or self._is_clang_cl:
            suffix += "_static" if not self.options.shared else ""
        suffix += "d" if self.settings.os == "Windows" and self.settings.build_type == "Debug" else ""
        self.cpp_info.libs = [f"{prefix}png{suffix}"]
        if self.settings.os in ["Linux", "Android", "FreeBSD", "SunOS", "AIX"]:
            self.cpp_info.system_libs.append("m")

        # TODO: Remove after Conan 2.0
        self.cpp_info.names["cmake_find_package"] = "PNG"
        self.cpp_info.names["cmake_find_package_multi"] = "PNG"
