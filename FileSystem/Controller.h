#pragma once
#include "Inode.h"
#include "Superblock.h"
#include <string>
#include <vector>

struct StatInfo
{
	std::string name;
	int inode;
	int block;
	Inode::Type type;
	int blocks;
	int size;
	std::string parent;
};

// The whole disk, kept in memory and written to the disk file as one record.
struct DiskImage
{
	char magic[8];
	Superblock superblock;
	Inode inodes[INODE_MAX];
	int cwd; // inode id of the current directory
	char data[TOTAL_BLOCKS * BLOCK_SIZE];
};

// Every mutating command works on a copy of the image and only replaces
// the real one (and the disk file) when it finished without an error.
class Controller
{
	std::string diskPath;
	DiskImage image;

	void commit(const DiskImage&);

public:
	explicit Controller(const std::string& diskPath = "disk.bin");

	void mkdir(const std::string& name);
	void create(const std::string& name);
	std::string cd(const std::string& path); // returns the new current path
	std::vector<std::string> ls() const;
	StatInfo stat(const std::string& path) const;
	void echo(const std::string& path, const std::string& text);
	std::string cat(const std::string& path) const;
	void rm(const std::string& path);
	void rename(const std::string& path, const std::string& newName);
	void cp(const std::string& source, const std::string& destination);
	void mv(const std::string& source, const std::string& destination);
	void get(const std::string& localPath); // host file -> file system
	void put(const std::string& path);      // file system -> host file

	std::string currentPath() const;
};
