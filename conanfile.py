import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps
from conan.tools.files import copy

class RoR(ConanFile):
    name = "Rigs of Rods"
    settings = "os", "compiler", "build_type", "arch"
    default_options = {
        "ogre3d*:nodeless_positioning": "True",
        "ogre3d*:resourcemanager_strict": "off"
    }

    def layout(self):
        self.folders.generators = os.path.join(self.folders.build, "generators")

    def requirements(self):
        self.requires("angelscript/2.35.1")
        self.requires("discord-rpc/3.4.0@anotherfoxguy/stable")
        self.requires("fmt/10.1.1")
        self.requires("libcurl/8.2.1")
        self.requires("mygui/3.4.3@anotherfoxguy/stable")
        self.requires("ogre3d-caelum/2022.08@anotherfoxguy/stable")
        self.requires("ogre3d-pagedgeometry/2023.07@anotherfoxguy/stable")
        self.requires("ogre3d/14.1.2@anotherfoxguy/stable", force=True)
        self.requires("ois/1.4.1@rigsofrods/custom")
        self.requires("openal-soft/1.22.2@anotherfoxguy/patched")
        self.requires("openssl/3.1.2", force=True)
        self.requires("rapidjson/cci.20230929", force=True)
        self.requires("socketw/3.11.0@anotherfoxguy/stable")

        self.requires("libpng/1.6.40", override=True)
        self.requires("libwebp/1.3.2", override=True)
        self.requires("zlib/1.3", override=True)
        self.requires("xz_utils/5.4.5", override=True)

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
