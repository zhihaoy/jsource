import os
import sys
import subprocess

from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

        nt = os.name == 'nt'
        cfg = "Debug" if self.debug else "Release"

        cmake_args = [
            "-G", "Ninja",
            "-D", "CMAKE_C_COMPILER={}".format('clang-cl' if nt else 'clang'),
            "-D", "CMAKE_CXX_COMPILER={}".format('clang-cl' if nt else 'clang++'),
            "-D", "CMAKE_LIBRARY_OUTPUT_DIRECTORY={}".format(extdir),
            "-D", "CMAKE_RUNTIME_OUTPUT_DIRECTORY={}".format(extdir),
            "-D", "Python_EXECUTABLE={}".format(sys.executable),
            "-D", "BUILD_TESTING=OFF",
            "-D", "CMAKE_BUILD_TYPE={}".format(cfg),
            "-B", self.build_temp,
        ]

        if 'CONDA_BUILD' in os.environ:
            if not nt:
                cmake_args += [
                    "-D",
                    "CMAKE_EXE_LINKER_FLAGS_INIT=-Wl,-rpath={0} -L{0}".format(
                        os.path.join(os.environ['PREFIX'], 'lib')
                    ),
                    "-D",
                    "CMAKE_SHARED_LINKER_FLAGS_INIT=-Wl,-rpath={0} -L{0}".format(
                        os.path.join(os.environ['PREFIX'], 'lib')
                    ),
                ]

        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args
        )
        subprocess.check_call(
            ["cmake", "--build", self.build_temp]
        )


setup(
    ext_modules=[CMakeExtension("jruntime")],
    cmdclass={"build_ext": CMakeBuild},
)
