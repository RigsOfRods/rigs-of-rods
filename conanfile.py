import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps
from conan.tools.files import copy

class RoR(ConanFile):
    name = "Rigs of Rods"
    settings = "os", "compiler", "build_type", "arch"
    default_options = {
        "ogre3d*:resourcemanager_strict": "off",
        "ogre3d*:profiling": "True"
    }

    def configure(self):
        if self.settings.os == "Macos":
            self.options["ogre3d*"].profiling = False

    def layout(self):
        if self.settings.os == "Macos":
            self.folders.build = "build"
        self.folders.generators = os.path.join(self.folders.build, "generators")

    def requirements(self):
        if self.settings.os == "Macos":
            # On macOS (Apple Silicon), AngelScript is built from source with
            # arm64 callfunc support via tools/macos/build.sh, and Caelum's
            # x86-only Cg dependency is omitted on armv8.
            self.requires("libcurl/8.2.1")
            self.requires("fmt/10.0.0", force=True)
            self.requires("mygui/3.4.0@anotherfoxguy/stable")
            if self.settings.arch != "armv8":
                self.requires("ogre3d-caelum/0.6.3.1@anotherfoxguy/stable")
            self.requires("ogre3d/1.11.6.1@anotherfoxguy/stable", force=True)
            self.requires("ois/1.4.1@rigsofrods/custom")
            self.requires("openal-soft/1.24.3")
            self.requires("openssl/3.2.0", force=True)
            self.requires("rapidjson/cci.20211112", force=True)
            self.requires("socketw/3.11.0@anotherfoxguy/stable")

            self.requires("jasper/4.0.0", override=True)
            self.requires("libpng/1.6.40", override=True)
            self.requires("libwebp/1.3.0", override=True)
            self.requires("zlib/1.3", override=True)
            self.requires("zziplib/0.13.78@anotherfoxguy/stable", override=True)
        else:
            self.requires("angelscript/2.38.0")
            self.requires("discord-rpc/3.4.0@anotherfoxguy/stable")
            self.requires("libcurl/8.2.1")
            self.requires("fmt/12.2.0")
            self.requires("mygui/3.4.0@anotherfoxguy/stable")
            self.requires("ogre3d-caelum/0.6.3.1@anotherfoxguy/stable")
            self.requires("ogre3d/1.11.6.1@anotherfoxguy/stable", force=True)
            self.requires("ois/1.4.1@rigsofrods/custom")
            self.requires("openal-soft/1.24.3")
            self.requires("openssl/3.6.3", force=True)
            self.requires("rapidjson/cci.20211112", force=True)
            self.requires("socketw/3.11.0@anotherfoxguy/stable")

            self.requires("jasper/4.2.4", override=True)
            self.requires("libpng/1.6.58", override=True)
            self.requires("libwebp/1.6.0", override=True)
            self.requires("zlib/1.3.2", override=True)
            self.requires("zziplib/0.13.78@anotherfoxguy/stable", override=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()
        if self.settings.os == "Windows" and self.settings.build_type == "Release":
            deps.configuration = "RelWithDebInfo"
            deps.generate()

        for dep in self.dependencies.values():
            for f in dep.cpp_info.bindirs:
                self.cp_data(f)
            for f in dep.cpp_info.libdirs:
                self.cp_data(f)

    def cp_data(self, src):
        bindir = os.path.join(self.build_folder, "bin")
        copy(self, "*.dll", src, bindir, False)
        copy(self, "*.so*", src, bindir, False)
        copy(self, "*.dylib", src, bindir, False)
