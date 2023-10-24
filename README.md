# FileSystem
File system based on existing file systems

A simulated file system that lives in a single file (`disk.bin`) and is used through a console shell with Unix-like commands. The design follows real file systems (ext2/UNIX): a superblock with bitmaps, i-nodes and data blocks.

## Contents
1. [Build and creating the .exe](#build-and-creating-the-exe)
2. [Commands](#commands)
3. [How it works](#how-it-works)
4. [Code structure](#code-structure)
5. [Differences from the original version](#differences-from-the-original-version)
6. [Regression tests](#regression-tests)

## Build and creating the .exe

Compiled files (`.exe`, `.pdb`, `.ilk`) and `disk.bin` are not stored in the repository: build the program yourself, and `disk.bin` is created by the program when it first runs.

The project uses CMake, which builds the executable for the platform it is run on.

| Environment | Commands | Result |
|---|---|---|
| Linux / WSL | `cmake -S . -B build && cmake --build build` | `build/FileSystem` |
| Windows, Visual Studio or Build Tools (in a "Developer Command Prompt") | `cmake -S . -B build && cmake --build build --config Debug` | `build\Debug\FileSystem.exe` |
| Windows, Visual Studio GUI | File, Open, Folder (VS detects `CMakeLists.txt`), then Build | `out\build\...\FileSystem.exe` |
| Linux, cross-compiling for Windows (needs `mingw-w64`) | `x86_64-w64-mingw32-g++ -std=c++17 -O2 -static FileSystem/*.cpp -o FileSystem.exe` | `FileSystem.exe` |

Requirements: a C++17 compiler (`<filesystem>` is used). For a release build use `--config Release` instead of `Debug`.

There is no Windows compiler in the WSL environment used for development, so only the Linux binary was built there. Build the Windows `.exe` on Windows with one of the commands above.

Run it with `./build/FileSystem` (or `FileSystem.exe`). `disk.bin` is created in the working directory the program is started from.

## Commands

Paths: `name`, `folder/name`, `root/name`, `/name`, `..`.

| Command | Description |
|---|---|
| `mkdir <name>` | create a folder (from root only) |
| `create <name>` | create an empty file in the current directory |
| `cd <path>` / `CD` | enter a folder (`cd ..`, `cd root`) / go back to root |
| `ls` | list the current directory |
| `stat <path>` | i-node info: name, i-node number, block, type, number of blocks, size, parent |
| `echo <file> <text>` | write text to a file (replaces the whole content); the file is reallocated if the text does not fit |
| `cat <file>` | print a file |
| `rename <path> <new name>` | rename a file or folder |
| `rm <path>` | remove a file, or a folder with its content |
| `cp <source> <destination>` | copy a file; the destination is a new name, an existing file (overwritten) or a folder |
| `mv <source> <destination>` | move or rename a file |
| `get <local file>` | import a file from the computer into the current directory |
| `put <file>` | export a file to the computer (into the program's working directory, under the same name) |
| `help`, `exit` | help, quit |

The prompt shows the current directory: `root> `, `root/docs> `.

## How it works

### Disk parameters (`Superblock.h`)

| Constant | Value | Meaning |
|---|---|---|
| `BLOCK_SIZE` | 20 | bytes per block |
| `TOTAL_BLOCKS` | 100 | blocks on the disk (2000 B of data) |
| `INODE_MAX` | 20 | number of i-nodes, i.e. files and folders in total including root |
| `NAME_MAX_LENGTH` | 31 | maximum name length (`Inode.h`) |

The values are deliberately small so the limits (full disk, fragmentation) are easy to test. They are changed only here, but a disk file created with different values is not compatible (the program rejects it).

### Layout of `disk.bin`

`disk.bin` is one record of the `DiskImage` structure (3172 B):

| Part | Size | Content |
|---|---|---|
| `magic` | 8 B | `FSIMG001`, format marker |
| `superblock` | 120 B | i-node bitmap (20 B) and block bitmap (100 B), one byte (`bool`) per entry |
| `inodes[20]` | 20 x 52 B | i-node table |
| `cwd` | 4 B | i-node number of the current directory (persisted, so it survives a restart) |
| `data` | 100 x 20 B | data blocks |

The whole disk is loaded into memory at startup and written back after every successful command.

### I-node (`Inode`)

| Field | Meaning |
|---|---|
| `name[32]` | name (at most 31 characters plus the terminating zero) |
| `dataBlockLocation` | number of the first data block |
| `numberOfBlocks` | how many blocks are allocated |
| `size` | how many bytes are actually stored (file size) |
| `parent` | i-node number of the containing folder (`-1` for root and free i-nodes) |
| `type` | `0` folder, `1` file |

`Inode` is written as raw bytes, so it must stay a plain type (no `std::string`, no pointers). A `static_assert` in `Controller.cpp` enforces this.

### Hierarchy

Folders have no list of entries. Every i-node knows its parent (`parent`), and `ls` walks all used i-nodes and prints those whose `parent` is the current directory, in i-node order. I-node 0 is root (named `root`, no parent). Depth is limited to 2: `mkdir` works only from root, so folders are always directly under root, and files live in root or in a folder.

### Allocation (`Superblock`)

- Used i-nodes and used blocks are tracked in bitmaps. I-node 0 and block 0 are reserved (block 0 means "no data"), so 99 blocks and 19 i-nodes are available.
- A file occupies **contiguous** blocks. The first hole that is long enough is used (`findFreeRun`, first-fit).
- If there is no such hole but there are enough free blocks in total, the disk is **defragmented** (`defragment`): all files are sorted by their current position and their data is moved (`memmove`) towards the start of the disk one after another, fixing `dataBlockLocation` and the bitmap. Then the search is repeated.
- If there is still no room: `There is not enough space.`
- `create` reserves 5 blocks (100 B), `mkdir` 1 block, and `get` and `echo` grow as needed (`ceil(bytes / 20)` blocks). Newly allocated blocks are zeroed.
- The free block count and free space are computed from the bitmap, so they can never disagree with the actual state.

### Transactions

Every command that modifies the disk works on a **copy** of `DiskImage`. Only if it finishes without an error is the copy written to the file (first to `disk.bin.tmp`, then `rename`d over `disk.bin`) and made the current state. If something fails in the middle of an operation (for example space runs out after part of the work is done), the disk stays as it was. That is why a failed `echo` does not destroy the old content.

### Path resolution (`parsePath`, `resolve`)

A path is split on `/`. If it starts with `/` or its first part is `root`, resolution starts at root; otherwise at the current directory. Every middle part must be an existing folder. `.` is the same folder and `..` is the parent (root stays root). The last part is the item name; if it is `..`, `.` or just `root`, the path denotes a folder. This makes `cat docs/c.txt`, `cd ..` and `cp a.txt /b.txt` work.

Names are validated (`validateName`): not empty, at most 31 characters, no `/`, and not `.`, `..` or `root`. Two items in the same folder cannot have the same name.

### Commands

- **mkdir**: root only (`Maximum depth is 2.`). Creates a folder and gives it 1 block.
- **create**: creates a file of size 0 with 5 blocks in the current directory.
- **cd**: resolves the path, which must be a folder. Writes `cwd`. `CD` is the same as `cd root`.
- **stat**: prints the i-node fields. "Parent directory" is the item's real parent.
- **echo**: `writeData`. If the text fits into the already allocated blocks, it is written in place (the rest is zeroed) and `size` is set to the text length. If it does not fit, the old blocks are released, `ceil(n/20)` new ones are allocated (with defragmentation if needed), and the same i-node gets the new values. The content is always **replaced**, never appended.
- **cat**: prints exactly `size` bytes starting at `dataBlockLocation`. A file that was never written prints an empty line.
- **rm**: for a file, frees its blocks and i-node. For a folder, first recursively removes everything in it. Root cannot be removed. If the removed folder is the current one, `cwd` goes back to root.
- **rename**: changes the name, checking the name and duplicates in the same folder.
- **cp / mv** (`transfer`):
  - The source must be a file (folders can be neither copied nor moved).
  - If the destination is an existing folder, the file goes into it under its own name.
  - If the destination is an existing file, its content is overwritten (the same source and destination is an error).
  - Otherwise a new file with the given name is created.
  - `mv` to a name that does not exist only changes `name` and `parent` in the i-node, without copying data. `mv` over an existing file overwrites the destination and removes the source.
- **get**: reads a local file (binary), creates a file in the current directory named after the local file (without the path) and stores the content. A file bigger than the whole disk is rejected. If the name already exists, it is an error.
- **put**: writes exactly `size` bytes to a local file named like the item (an existing file is overwritten).

### Disk validation on load

At startup `disk.bin` is checked: exact size, `magic`, valid root and `cwd`, and for every used i-node: the name is zero-terminated, the blocks are within the disk, `size` fits into the allocated blocks and `parent` is an existing folder. If a check fails, the program prints `The disk file is corrupted...` or `...invalid format...` and exits, without deleting the file.

### Shell (`UI`)

A line is split into the command (first word) and the rest. The command is compared whole (`ls` is not `lsxyz`) and is case-sensitive (`cd` vs `CD`). Commands with one argument take the whole rest of the line (names may contain spaces). `cp`, `mv` and `rename` take the first word and the rest. `echo` takes the file and the rest as the text. A wrong number of arguments gives `Usage: ...`, an unknown command gives a message pointing to `help`. Errors are printed and the loop continues. The loop ends on `exit` or at the end of input.

## Code structure

| File | Role |
|---|---|
| `FileSystem/Superblock.*` | disk constants, bitmaps, finding a free i-node and contiguous blocks |
| `FileSystem/Inode.*` | the i-node (fixed layout, no `std::string`) |
| `FileSystem/Controller.*` | `DiskImage`, loading/saving the disk and all file system logic (paths, allocation, defragmentation, commands) |
| `FileSystem/UI.*` | shell: command parsing and output |
| `FileSystem/Source.cpp` | `main` |
| `CMakeLists.txt` | build |
| `tests/` | regression tests |

`Controller` prints nothing (it returns values or throws `std::runtime_error`), so it can be used and tested without a console.

## Differences from the original version

The core logic is the same: i-nodes and bitmaps, `parent` as the hierarchy, contiguous first-fit blocks, `create` = 5 blocks, `mkdir` = 1 block, depth 2, the same commands and the same i-node and block numbers in the output (verified by comparing the output of the original `.exe` with the new program).

Intentional differences:
- The disk persists between runs (the original wiped it on every start).
- The file size is stored in the i-node, so there is no `'0'` terminator (`cat` does not print a trailing `0`, and text containing the digit 0 is no longer cut off).
- `Inode` is a plain type (the original wrote a `std::string` as raw bytes).
- Paths work in all commands; `cp` creates the destination if it does not exist; `mv` to a new name only renames.
- `echo` does not delete the old file before making sure the new content fits; the block count is `ceil(n/20)` (original `n/20 + 2`), so scenario 06 uses 7 blocks instead of 8.
- Defragmentation is implemented (it was commented out in the original) and commands are atomic.
- Errors: `std::runtime_error` instead of `std::exception("...")` (which only compiles with MSVC), a single language for messages, and no crash on bad input (the original crashed on `cp` without a `/` and printed forever on `cat` of an empty file). The original `.exe` prints its labels in Serbian.
- Fixed bug: in the original, the file in i-node 6 was not visible in `ls` on a full disk (`f6`).
- Added: name validation, `help`, a prompt with the path, `cd ..`, the `Size` field in `stat`, disk integrity checks.

## Regression tests

```
tests/run.sh build/FileSystem            # compare with tests/golden
tests/run.sh build/FileSystem --record   # re-record tests/golden
```

Each scenario in `tests/scenarios` (one command per line) is run in an empty temporary folder, and the program output plus any files it left behind (`put`) are compared. A `#restart` line in a scenario restarts the program over the same disk (persistence test). An optional `<scenario>.setup` shell script is run in the temporary folder before the scenario (used to create large local files and corrupted disk files). Files larger than 256 bytes are compared by size and checksum. `tests/golden` holds the reviewed expected output; the intended changes compared to the original version are listed above. The tests can also be run against a Windows `.exe` from WSL (`tests/run.sh build/Debug/FileSystem.exe`).
