from conans import ConanFile, CMake

class ScientistConan(ConanFile):
    name = "scientist"
    version = "1.0.0"
    settings = "os", "compiler", "arch", "build_type"
    exports_sources = "include/*", "CMakeLists.txt", "benchmark/*", "test/*"
    no_copy_source = True

    def build(self):
        cmake = CMake(self)
        cmake.definitions['BENCHMARK_ENABLE_TESTING'] = 'OFF'
        cmake.configure()
        cmake.build()
        cmake.test()

    def package(self):
        self.copy("*.h")

    def package_id(self):
        self.info.header_only()
