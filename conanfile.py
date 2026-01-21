import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps
from conan.tools.files import copy

class RoR(ConanFile):
    name = "Rigs of Rods"
    settings = "os", "compiler", "build_type", "arch"

    def layout(self):
        self.folders.generators = os.path.join(self.folders.build, "generators")

    def requirements(self):
        self.requires("angelscript/2.35.1")
        self.requires("discord-rpc/3.4.0@anotherfoxguy/stable")
        self.requires("libcurl/8.2.1")
        self.requires("fmt/10.1.1")
        self.requires("mygui/3.4.0@anotherfoxguy/stable")
        self.requires("ogre3d-caelum/0.6.3.1@anotherfoxguy/stable")
        self.requires("ogre3d-pagedgeometry/1.2.0@anotherfoxguy/stable")
        self.requires("ogre3d/1.11.6.1@anotherfoxguy/stable", force=True)
        self.requires("ois/1.4.1@rigsofrods/custom")
        self.requires("openal-soft/1.22.2")
        self.requires("openssl/3.6.0", force=True)
        self.requires("rapidjson/cci.20211112", force=True)
        self.requires("socketw/3.11.0@anotherfoxguy/stable")

        self.requires("jasper/4.2.4", override=True)
        self.requires("libpng/1.6.39", override=True)
        self.requires("libwebp/1.3.2", override=True)
        self.requires("zlib/1.2.13", override=True)

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
