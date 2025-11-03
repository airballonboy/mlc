# mlc

mlang is a small programming lanuage aimed at making console programs and games,
It's currently just a project for me to learn how programming languages work,
But if you wan't to use it feel free.

## Build Guide
To build and use the mlang compiler (mlc) you need to have cmake and gcc and a build system (make, ninja, msvc)

### Linux
```console
$ mkdir build
$ cd build
$ cmake ..
$ make     # or ninja
$ ./mlc input.mlang [-o output.mlang] [-run]
```
### Windows
```console
$ mkdir build
$ cd build
$ cmake ..
$ ninja   # or open project in visual studio
$ ./mlc.exe input.mlang [-o output.mlang] [-run]
```
