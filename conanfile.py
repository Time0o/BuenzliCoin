from conans import ConanFile, CMake, tools

class BuenzliCoindConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"

    requires = [
        ('boost/1.78.0'),
        ('catch2/2.13.7'),
        ('fmt/8.0.1'),
        ('nlohmann_json/3.10.4'),
        ('openssl/1.1.1m'),
        ('pybind11/2.8.1'),
        ('pybind11_json/0.2.11'),
        ('tomlplusplus/2.5.0')
    ]

    generators = "cmake_find_package"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
