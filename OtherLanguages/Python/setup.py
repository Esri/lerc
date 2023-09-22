import setuptools

from glob import glob
from os.path import basename, exists, join, getmtime
from shutil import copyfile

readme_path = join("lerc", "README.md")

try:
    with open(readme_path, "r") as fh:
        long_description = fh.read()
except Exception:
    long_description = "Limited Error Raster Compression"

# Using MANIFEST.in doesn't respect relative paths above the package root.
# Instead, inspect the location and copy in the binaries if newer.
BINARY_TYPES = ["*.dll", "*.lib", "*.so*", "*.dylib"]
PLATFORMS = ["Linux", "MacOS", "windows"]
for platform in PLATFORMS:
    platform_dir = join("..", "..", "bin", platform)
    for ext in BINARY_TYPES:
        input_binaries = glob(join(platform_dir, ext))
        for input_binary in input_binaries:
            output_binary = join("lerc", basename(input_binary))
            if not exists(output_binary) or getmtime(input_binary) > getmtime(output_binary):
                copyfile(input_binary, output_binary)

setuptools.setup(
    name="pylerc",
    version="4.0",
    author="esri",
    author_email="python@esri.com",
    description="Limited Error Raster Compression",
    long_description=long_description,
    long_description_content_type="text/markdown",
    license="Apache 2",
    url="https://github.com/Esri/lerc",
    packages=setuptools.find_packages(),
    install_requires=["numpy"],
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: Apache Software License",
        "Operating System :: OS Independent",
    ],
    package_data={"lerc": BINARY_TYPES},
    python_requires=">=3.6",
    zip_safe=False,
)
