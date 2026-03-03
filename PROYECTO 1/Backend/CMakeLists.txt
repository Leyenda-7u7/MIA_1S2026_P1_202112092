cmake_minimum_required(VERSION 3.16)
project(Proyecto1Backend LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(app
    src/main.cpp
    src/DiskManager.cpp
    src/ext2/Bitmap.cpp
    src/CommandParser.cpp

    src/commands/mkdisk.cpp
    src/commands/rmdisk.cpp
    src/commands/fdisk.cpp
    src/commands/mount.cpp
    src/commands/mkfs.cpp
    src/commands/login.cpp
    src/commands/cat.cpp
    src/commands/mkgrp.cpp
    src/commands/rmgrp.cpp
    src/commands/mkusr.cpp
    src/commands/chgrp.cpp
    src/commands/rmusr.cpp
    src/commands/mkfile.cpp
    src/commands/mkdir.cpp
)

target_include_directories(app PRIVATE include)
