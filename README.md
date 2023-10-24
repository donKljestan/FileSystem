# File System Simulator

A university C++ project implementing a small file system inside a single binary file (`disk.bin`). It is inspired by traditional UNIX-style file systems and provides a command-line interface with familiar file and directory operations.

## How it works

The program treats `disk.bin` as a virtual disk. It stores a superblock, inode table and fixed-size data blocks, and uses bitmaps to track which inodes and blocks are allocated.

- **Inodes** store file and directory metadata, including names, parent directories, sizes and data block locations.
- **Block allocation** uses a first-fit search for contiguous free blocks. When free space is fragmented, the implementation can compact the data blocks and update inode locations.
- **Directories** are represented through parent inode references. The implementation supports a root directory and one level of subdirectories.
- **Persistence** is provided by loading the disk image on startup and saving changes back to it. Modifying operations are prepared on a copy of the disk image before being committed.

The virtual disk is intentionally small (100 blocks of 20 bytes and 20 inodes), making allocation and disk-space limitations easy to observe.

## Commands

| Command | Description |
| --- | --- |
| `mkdir`, `create` | Create a directory or file |
| `cd`, `ls`, `stat` | Navigate and inspect files |
| `echo`, `cat` | Write and read file contents |
| `cp`, `mv`, `rename`, `rm` | Manage files and directories |
| `get`, `put` | Import and export files between the host and virtual disk |
| `help`, `exit` | Show commands or exit |

Example session:

```text
root> mkdir docs
root> cd docs
root/docs> create notes.txt
root/docs> echo notes.txt Hello from the virtual disk
root/docs> cat notes.txt
Hello from the virtual disk
root/docs> ls
```

## Build and run

Requires a C++17 compiler and CMake.

**Linux / WSL:**

```bash
cmake -S . -B build
cmake --build build
./build/FileSystem
```

**Windows (Visual Studio / MSVC):**

```powershell
cmake -S . -B build
cmake --build build --config Debug
.\build\Debug\FileSystem.exe
```

The program creates `disk.bin` in its working directory if no existing disk image is found.

## Project structure

- `FileSystem/Superblock.*` - disk parameters, allocation bitmaps and free-block search
- `FileSystem/Inode.*` - file and directory metadata
- `FileSystem/Controller.*` - disk image, file operations, path resolution and defragmentation
- `FileSystem/UI.*` - command-line interface
- `FileSystem/Source.cpp` - application entry point
- `tests/` - command scenarios and regression tests

To run the regression tests on Linux / WSL after building:

```bash
tests/run.sh build/FileSystem
```

## Background

This is a university coursework project. It is a simplified file system simulation, not a kernel file system or a replacement for a production file system.