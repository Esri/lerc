import setuptools
from glob import glob
from os.path import basename, exists, join, getmtime
from shutil import copyfile

# Forces the wheel to be platform-specific (e.g., win_amd64)
# but compatible with any Python 3 version (py3-none).
try:
    from wheel.bdist_wheel import bdist_wheel as _bdist_wheel
    class bdist_wheel(_bdist_wheel):
        def finalize_options(self):
            _bdist_wheel.finalize_options(self)
            self.root_is_pure = False
            
        def get_tag(self):
            _, _, plat = _bdist_wheel.get_tag(self)
            return "py3", "none", plat
    cmdclass = {'bdist_wheel': bdist_wheel}
except ImportError:
    cmdclass = {}

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
    version="4.1",
    author="esri",
    author_email="python@esri.com",
    description="Limited Error Raster Compression",
    long_description=long_description,
    long_description_content_type="text/markdown",
    license="Apache 2",
    url="https://github.com/Esri/lerc",
    packages=setuptools.find_packages(),
    install_requires=["numpy >=2.3.0,<3"],
    cmdclass=cmdclass,
    classifiers=[
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "License :: OSI Approved :: Apache Software License",
    ],
    package_data={"lerc": BINARY_TYPES},
    python_requires=">=3.11",
    zip_safe=False,
)
