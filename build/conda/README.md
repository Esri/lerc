## conda package

To build the conda package:
1. Place the appropriate compiled binaries in `/OtherLanguages/Python/lerc/`
    - For windows, this is the `.lib` and `.dll` files
	- For Linux, this is the `.so` files
	- For OSX, this is the `.dylib` files
2. In this directory, run `conda build lerc --output-folder /some/output/folder --python 3.6` to build the python3.6 package for the architecture of your host computer
3. Repeat step 2 with `--python 3.7`, then again with `--python 3.8` (Or whatever python versions you want to release in the future)
4. Repeat steps 1-3 for the other architectures you want to release for (Windows, Linux, OSX)
