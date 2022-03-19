from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

class my_build_ext(build_ext):
    def build_extensions(self):
        for e in self.extensions:
            e.extra_compile_args = ['--pedantic']
            e.extra_link_args = ['-lgomp']

        build_ext.build_extensions(self)

setup(name = "mymath",
      version = "0.1",
      ext_modules = [Extension("segm", ["common.cpp", "Enclosure.cpp", "ImageNode.cpp", "LineSpacing.cpp", "PageSegmenter.cpp", "Reflow.cpp", "Xycut.cpp", "segm.cpp"])],
      cmdclass = {'build_ext': my_build_ext},
      );