## Dependencies

You will need, at a minimum, the following dependencies to build Celeste.

* Qt
  * QtMultimedia
  * QtSpeech (Linux only)
* cmake
* A C++ compiler such as g++, clang, or MSVC

### Ubuntu

On Ubuntu, dependencies are provided by installing the following packages.

* cmake
* g++
* qt5-default
* libqt5texttospeech5-dev (Linux only)
* qtmultimedia5-dev


A backend for QtMultimedia must be installed for it to work.

* libqt5multimedia5-plugins

## Compilation

Build Celeste by performing the following steps from within the top level directory of the repository.

```
mkdir build
cd build
cmake ..
make
make install
```

Note that the `make install` command will require administrator privileges.