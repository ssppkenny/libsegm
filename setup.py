from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext
import sys, os, subprocess

cmake_cmd_args = []

class CMakeExtension(Extension):
    def __init__(self, name, cmake_lists_dir=".", **kwa):
        Extension.__init__(self, name, sources=[], **kwa)
        self.cmake_lists_dir = os.path.abspath(cmake_lists_dir)


class cmake_build_ext(build_ext):
    def build_extensions(self):
        # Ensure that CMake is present and working
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError('Cannot find CMake executable')

        for ext in self.extensions:

            extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
            cfg = 'Debug'

            cmake_args = [
                '-DCMAKE_BUILD_TYPE=%s' % cfg,
                # Ask CMake to place the resulting library in the directory
                # containing the extension
                '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), extdir),
                # Other intermediate static libraries are placed in a
                # temporary build directory instead
                '-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_{}={}'.format(cfg.upper(), self.build_temp),
                # Hint CMake to use the same Python executable that
                # is launching the build, prevents possible mismatching if
                # multiple versions of Python are installed
                '-DPYTHON_EXECUTABLE={}'.format(sys.executable),
                # Add other project-specific CMake arguments if needed
                # ...
            ]

            # We can handle some platform-specific settings at our discretion
            cmake_args += cmake_cmd_args

            if not os.path.exists(self.build_temp):
                os.makedirs(self.build_temp)

            # Config
            subprocess.check_call(['cmake', ext.cmake_lists_dir] + cmake_args,
                                  cwd=self.build_temp)

            # Build
            subprocess.check_call(['cmake', '--build', '.', '--config', cfg],
                                  cwd=self.build_temp)


setup(name = "segm",
      version = "0.1",
      cmdclass={'build_ext': cmake_build_ext},
      ext_modules = [CMakeExtension("segm")]
      );
