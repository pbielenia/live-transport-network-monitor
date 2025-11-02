from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, cmake_layout


class ConanPackage(ConanFile):
    name = 'network-monitor'
    version = "0.1.0"
    settings = "os", "compiler", "build_type", "arch"
    requires = ("boost/1.80.0",
                "libcurl/7.85.0",
                "nlohmann_json/3.11.2",
                "openssl/3.6.0",
                "zlib/1.2.13",
                )
    default_options = {"boost/*:shared": False}

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        toolchain.generator = "Ninja"

        if self.settings.build_type == "Debug":
            toolchain.cache_variables["CMAKE_EXPORT_COMPILE_COMMANDS"] = "ON"

        toolchain.generate()

    def layout(self):
        cmake_layout(self)
