from setuptools import setup, Extension

setup(name = "segm",
      version = "0.1",
      ext_modules = [Extension("segm", ["segm.cpp"])]
      );
