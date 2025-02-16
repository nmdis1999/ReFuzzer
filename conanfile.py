from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout


class SmartCompilerTesting(ConanFile):
    name = "Smart Compiler Testing"
    version = "0.0.1"
    settings = "os", "compiler", "build_type", "arch"
    export_sources = "CMakeLists.txt", "src/*"
    requires = ["nlohmann_json/3.11.3", "zlib/1.3.1", "openssl/3.3.2", "libcurl/8.11.1"]

    def layout(self):
        cmake_layout(self)
        self.folders.source = "src"
        self.folders.generators = "build/generators"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generate()
        td = CMakeDeps(self)
        td.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
