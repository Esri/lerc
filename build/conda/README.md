## conda package

To build the conda package:
1. From a standalone Python session, run `python setup.py build` in `/OtherLanguages/Python/lerc/`, so that it will copy the binaries.
2. In this directory, run `conda build lerc --output-folder /some/output/folder` to build a single unified Python package for all platforms.
