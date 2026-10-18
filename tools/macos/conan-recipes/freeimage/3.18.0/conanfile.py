from conan import ConanFile
from conan.tools.build import check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import apply_conandata_patches, copy, export_conandata_patches, get, rmdir
import os

required_conan_version = ">=1.53.0"


class FreeImageConan(ConanFile):
    name = "freeimage"
    description = "Open Source library project for developers who would like to support popular graphics image formats"\
                  "like PNG, BMP, JPEG, TIFF and others as needed by today's multimedia applications."
    homepage = "https://freeimage.sourceforge.io"
    url = "https://github.com/conan-io/conan-center-index"
    license = "FreeImage", "GPL-3.0-or-later", "GPL-2.0-or-later"
    topics = ("image", "decoding", "graphics")

    package_type = "library"
    settings = "os", "arch", "compiler", "build_type"
    options = {
        "shared": [True, False],
        "fPIC": [True, False],
        "with_jpeg": [False, "libjpeg", "libjpeg-turbo", "mozjpeg"],
        "with_png": [True, False],
        "with_tiff": [True, False],
        "with_jpeg2000": [True, False],
        "with_openexr": [True, False],
        "with_eigen": [True, False],
        "with_webp": [True, False],
        "with_raw": [True, False],
        "with_jxr": [True, False],
    }
    default_options = {
        "shared": False,
        "fPIC": True,
        "with_jpeg": "libjpeg",
        "with_png": True,
        "with_tiff": True,
        "with_jpeg2000": True,
        "with_openexr": True,
        "with_eigen": True,
        "with_webp": True,
        "with_raw": True,
        "with_jxr": True,
    }

    short_paths = True

    def export_sources(self):
        copy(self, "CMakeLists.txt", self.recipe_folder, self.export_sources_folder)
        export_conandata_patches(self)

    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC

    def configure(self):
        if self.options.shared:
            self.options.rm_safe("fPIC")
        self.output.warning("G3 plugin and JPEGTransform are disabled.")
        if bool(self.options.with_jpeg):
            if self.options.with_tiff:
                self.options["libtiff"].jpeg = self.options.with_jpeg

    def layout(self):
        cmake_layout(self, src_folder="src")

    def requirements(self):
        self.requires("zlib/[>=1.2.11 <2]")
        if self.options.with_jpeg == "libjpeg":
            self.requires("libjpeg/9e")
        elif self.options.with_jpeg == "libjpeg-turbo":
            self.requires("libjpeg-turbo/3.0.2")
        elif self.options.with_jpeg == "mozjpeg":
            self.requires("mozjpeg/4.1.1")
        if self.options.with_jpeg2000:
            self.requires("openjpeg/2.5.2")
        if self.options.with_png:
            self.requires("libpng/[>=1.6 <2]")
        if self.options.with_webp:
            self.requires("libwebp/1.3.2")
        if self.options.with_tiff or self.options.with_openexr:
            # can't upgrade to openexr/3.x.x because plugin tiff requires openexr/2.x.x header files
            self.requires("openexr/2.5.7")
        if self.options.with_raw:
            # can't upgrade to libraw >= 0.21 (error: no member named 'shot_select' in 'libraw_output_params_t')
            self.requires("libraw/0.20.2")
        if self.options.with_jxr:
            self.requires("jxrlib/cci.20170615")
        if self.options.with_tiff:
            self.requires("libtiff/4.6.0")

    def validate(self):
        if self.settings.compiler.get_safe("cppstd"):
            check_min_cppstd(self, "11")

    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.variables["FREEIMAGE_FOLDER"] = self.source_folder.replace("\\", "/")
        tc.variables["FREEIMAGE_WITH_JPEG"] = bool(self.options.with_jpeg)
        tc.variables["FREEIMAGE_WITH_OPENJPEG"] = self.options.with_jpeg2000
        tc.variables["FREEIMAGE_WITH_PNG"] = self.options.with_png
        tc.variables["FREEIMAGE_WITH_WEBP"] = self.options.with_webp
        tc.variables["FREEIMAGE_WITH_OPENEXR"] = self.options.with_openexr
        tc.variables["FREEIMAGE_WITH_RAW"] = self.options.with_raw
        tc.variables["FREEIMAGE_WITH_JXR"] = self.options.with_jxr
        tc.variables["FREEIMAGE_WITH_TIFF"] = self.options.with_tiff
        tc.generate()
        cd = CMakeDeps(self)
        cd.generate()

    def _patch_sources(self):
        apply_conandata_patches(self)
        rmdir(self, os.path.join(self.source_folder, "Source", "LibPNG"))
        rmdir(self, os.path.join(self.source_folder, "Source", "LibTIFF4"))
        rmdir(self, os.path.join(self.source_folder, "Source", "LibOpenJPEG"))
        rmdir(self, os.path.join(self.source_folder, "Source", "LibJXR"))
        rmdir(self, os.path.join(self.source_folder, "Source", "LibWebP"))
        rmdir(self, os.path.join(self.source_folder, "Source", "LibRawLite"))
        rmdir(self, os.path.join(self.source_folder, "Source", "OpenEXR"))

    def build(self):
        self._patch_sources()
        cmake = CMake(self)
        cmake.configure(build_script_folder=os.path.join(self.source_folder, os.pardir))
        cmake.build()

    def package(self):
        for license in ["license-fi.txt", "license-gplv3.txt", "license-fi.txt"]:
            copy(self, license, src=self.source_folder, dst=os.path.join(self.package_folder, "licenses"))
        cmake = CMake(self)
        cmake.install()

    def package_info(self):
        def imageformats_deps():
            components = []
            components.append("zlib::zlib")
            if self.options.with_jpeg == "libjpeg":
                components.append("libjpeg::libjpeg")
            elif self.options.with_jpeg == "libjpeg-turbo":
                components.append("libjpeg-turbo::jpeg")
            elif self.options.with_jpeg == "mozjpeg":
                components.append("mozjpeg::libjpeg")
            if self.options.with_jpeg2000:
                components.append("openjpeg::openjpeg")
            if self.options.with_png:
                components.append("libpng::libpng")
            if self.options.with_webp:
                components.append("libwebp::libwebp")
            if self.options.with_openexr or self.options.with_tiff:
                components.append("openexr::openexr")
            if self.options.with_raw:
                components.append("libraw::libraw")
            if self.options.with_jxr:
                components.append("jxrlib::jxrlib")
            if self.options.with_tiff:
                components.append("libtiff::libtiff")
            return components

        self.cpp_info.components["FreeImage"].libs = ["freeimage"]
        self.cpp_info.components["FreeImage"].requires = imageformats_deps()
        self.cpp_info.components["FreeImagePlus"].libs = ["freeimageplus"]
        self.cpp_info.components["FreeImagePlus"].requires = ["FreeImage"]

        if not self.options.shared:
            self.cpp_info.components["FreeImage"].defines.append("FREEIMAGE_LIB")
