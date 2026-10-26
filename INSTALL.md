
Installation
============

Build Prerequisites
-------------------
  - GCC or Intel Compiler for Linux / macOS / Unix.
  - MinGW or MS VC++ for Windows.
  - (optionally) gettext library for internationalization
  - (optionally) OpenSSL for optimized algorithms

Build and install
-----------------
To compile and install the program use command
```sh
./configure && make && make install
```

The compiled program and library can be tested by command `make test test-lib`

To compile using MS VC++, take the project file from /win32/vc-2010/ directory.

Enabling features
-----------------
RHash can use optimized algorithms of MD5, SHA1, SHA2 from the OpenSSL library.
To link OpenSSL at run-time (preffered way), configure RHash as
```sh
./configure --enable-openssl-runtime
```
To link it at load-time, use options
```sh
./configure --enable-openssl --disable-openssl-runtime
```

Internationalization support can be compiled and installed by commands
```sh
./configure --enable-gettext
make install install-gmo
```

Run `./configure --help` for a full list of configuration options.

Building an OS native package
-----------------------------
When building a package for an OS Repository, one should correctly specify system directories, e.g.:
```sh
./configure --sysconfdir=/etc --exec-prefix=/usr
```

Example of installing RHash with shared and static LibRHash library:
```sh
./configure --enable-lib-static
make install install-lib-so-link
```


Optional Installation (CMake)
=============================

Build Prerequisites
-------------------
  - GCC or Intel Compiler for Linux / macOS / Unix.
  - MinGW or MS VC++ for Windows.
  - CMake 3.1 or later
  - (optionally) gettext library for internationalization
  - (optionally) OpenSSL for optimized algorithms

  Build and install
-----------------
To compile and install the program use command
```sh
cmake . -DCMAKE_BUILD_TYPE=Release && cmake --build . && cmake --build . --target install
```

Enabling features
-----------------
RHash can use optimized algorithms of MD5, SHA1, SHA2 from the OpenSSL library.
To link OpenSSL at run-time (preffered way), configure RHash as
```sh
cmake . -DOPENSSL_RUNTIME=ON
```
To link it at load-time, use options
```sh
cmake . -DUSE_OPENSSL=ON
```

Internationalization support can be compiled and installed by commands
```sh
cmake . -DUSE_GETTEXT=ON
make install install-gmo
```

Building an OS native package
-----------------------------
When building a package for an OS Repository, one should correctly specify system directories, e.g.:
```sh
cmake . -DCMAKE_INSTALL_PREFIX=/usr
```

Example of installing RHash with shared LibRHash library:
```sh
cmake . -DBUILD_SHARED_LIBS=ON
```
