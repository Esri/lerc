## conda package

To build the conda package:
1. Download the pre-compiled binaries from the GitHub Release assets. Create a bin/ directory in the repository root and copy the platform folders (windows, linux, macOS) into it.
2. From a standalone Python session, run `python setup.py build` in `/OtherLanguages/Python`, so that it will copy the binaries.
3. In this directory, run `conda build lerc --output-folder /some/output/folder` to build a single unified Python package for all platforms.
