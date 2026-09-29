# Building on Ubuntu 24.04

These notes describe how to build Unreal Engine 4.18 and the driving simulator on Ubuntu 24.04. They follow the [Carla 0.8.4 Linux build instructions](https://carla.readthedocs.io/en/0.8.4/how_to_build_on_linux/) with the following changes.
## Prerequisites

The package installation command in the vanilla Carla instructions fails on Ubuntu 24.04 because `clang-3.9` is no longer available through apt. This following command will fail:

```bash
sudo apt-get install build-essential clang-3.9 git cmake ninja-build python3-requests python-dev tzdata sed curl wget unzip autoconf libtool
```

Remove the `clang-3.9` spec, and manually install clang 3.9.1 from the LLVM release archive instead.

```bash
# Download Clang 3.9.1 (last 3.9.x release)
wget https://releases.llvm.org/3.9.1/clang+llvm-3.9.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz
# Extract to /opt
sudo tar -xf clang+llvm-3.9.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz -C /opt/
sudo mv /opt/clang+llvm-3.9.1-x86_64-linux-gnu-ubuntu-16.04 /opt/clang-3.9
```

The `sudo update-alternatives` step in the Carla instructions may also fail because its target directories do not exist. The setup scripts look for `clang-3.9` and `clang++-3.9` by name, so create symlinks with those names instead.

```bash
# Create symlinks
sudo ln -s /opt/clang-3.9/bin/clang /usr/local/bin/clang-3.9
sudo ln -s /opt/clang-3.9/bin/clang++ /usr/local/bin/clang++-3.9
# Add to PATH (optional)
echo 'export PATH="/opt/clang-3.9/bin:$PATH"' >> ~/.bashrc
```

If clang reports that `libtinfo` is missing, install `libtinfo5` as described in [this Ask Ubuntu answer](https://askubuntu.com/questions/1531760/how-to-install-libtinfo5-on-ubuntu24-04).

## Building Unreal Engine 4.18

Follow the [Unreal Engine Linux quick start](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-quick-start?application_version=4.27) to get access to the Unreal Engine GitHub repository. Then either pull the 4.18 branch or download the zipped release. Replace `Commit.gitdeps.xml` in the repository with the file attached to the release, as described in the [4.18.3 release notes](https://github.com/EpicGames/UnrealEngine/releases/tag/4.18.3-release). Without this replacement, `Setup.sh` fails with the following error.

```
Failed to download 'http://cdn.unrealengine.com/dependencies/UnrealEngine-3432852-0c2034138d8d407992169bf55a2d5775/01fe657d73f45b70239fd30d1d42b6a0d7decbbb': The remote server returned an error: (403) Forbidden. (WebException)
```

Make the following edits to `Engine/Build/BatchFiles/Linux/Setup.sh`.

* Remove `clang-3.9` from `DEPS`.
* Replace `mono-dmcs`, which is not available, with `mono-mcs`.

Build the engine from the repository's top-level directory.

```bash
./Setup.sh && ./GenerateProjectFiles.sh && make
```

If `make` fails because the `sysctl.h` header is missing, comment out that `#include`, since it is now redundant. See [this Ask Ubuntu answer](https://askubuntu.com/questions/1243362/unreal-fails-to-build-on-linux) for details.

## Building the driving simulator

Set `UE4_ROOT` to the path of the Unreal Engine repository, either in the current shell or in `~/.bashrc` followed by re-sourcing it.

```bash
export UE4_ROOT=<path to Unreal Engine repository>
```

### Setup.sh
Modify the `Setup.sh` script with the following changes:

* Remove the `wget` line that downloads Boost. The Boost download returns a 403 Forbidden error (see `README.md`), so download the Boost archive manually and place it in the directory the script expects.
* Comment out `-stdlib=libc++ -I../llvm-install/include/c++/v1` in `BOOST_CFLAGS`.
* Comment out `-stdlib=libc++ -L../llvm-install/lib` in `BOOST_LFLAGS`.
* Remove `-stdlib=libc++ -I$PWD/../llvm-install/include/c++/v1` from `CXXFLAGS`.
* Remove `-stdlib=libc++` from `LDFLAGS`.
* Remove `-stdlib=libc++ -I$PWD/../llvm-install/include/c++/v1` from `-DCMAKE_CXX_FLAGS`.

If the google-test build fails looking for `__builtin_fabsf128(__x)`, replace the `-DCMAKE_CXX_FLAGS` value with the following, which uses the clang 3.9 standard library rather than the system standard library.

```
-DCMAKE_CXX_FLAGS="-Wl,-L$PWD/../llvm-install/lib -stdlib=libc++ -nostdinc++ -I/opt/clang-3.9/include/c++/v1"
```

### Util/Protoc.sh

The script fails because its output directories do not exist. Add the following lines after `PROTOBUF_PY_OUT_DIR` and `PROTOBUF_CPP_OUT_DIR` are defined.

```bash
mkdir -p $PROTOBUF_PY_OUT_DIR
mkdir -p $PROTOBUF_CPP_OUT_DIR
```

### Util/cmake/CarlaServer/CMakeLists.txt

Comment out the following lines.

```cmake
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
include_directories("${CARLA_LIBCXX_INSTALL_PATH}/include/c++/v1")
link_directories("${CARLA_LIBCXX_INSTALL_PATH}/lib")
```

### Update.sh

A non-Gallant Lab user should user `Utils/download_from_gdrive.py` to get assets.

### Rebuild.sh

Building CarlaServer does not work reliably. Comment out `make debug && make release` on line 71, and copy the `Util/CarlaServer` directory from Zhang's existing build.

## Other build errors

If the `SIGSTKSZ` macro produces a non-constant error, replace `SIGSTKSZ` with `65536`.

If `xlocale.h` is not found, create it as a symlink to `/usr/include/locale.h`.

```bash
sudo ln -s /usr/include/locale.h /usr/include/xlocale.h
```

## Opening the project

Opening the project from the Unreal Editors's project browser may results in a "Missing Compiler; Install NullSourceCodeAccessor" message. Open the `.uproject` file directly instead.
