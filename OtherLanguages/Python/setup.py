import setuptools
import os

dir_path = os.path.dirname(os.path.realpath(__file__))
readme_path = os.path.join(dir_path, "..", "..", "README.md")

try:
    with open(readme_path, "r") as fh:
        long_description = fh.read()
except Exception:
    long_description = "Limited Error Raster Compression"

setuptools.setup(
    name="lerc",
    version="0.1.0",
    author="esri",
    author_email="python@esri.com",
    description="Limited Error Raster Compression",
    #long_description=long_description,
    #long_description_content_type="text/markdown",
    url="https://github.com/Esri/lerc",
    packages=setuptools.find_packages(),
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
    ],
    package_data={
        "lerc":["*.dll", "*.lib", "*.so", "*.dylib"]},
    python_requires='>=3.6')
