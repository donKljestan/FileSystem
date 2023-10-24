#pragma once
#include <string>

const int NAME_MAX_LENGTH = 31;

// Stored on disk as raw bytes, so it must stay trivially copyable (no std::string).
class Inode
{
	char name[NAME_MAX_LENGTH + 1];
	int dataBlockLocation; // first data block
	int numberOfBlocks;    // allocated blocks
	int size;              // used bytes
	int parent;            // inode id of the containing folder, -1 for root and unused inodes
	int type;

public:
	enum Type { Folder = 0, File = 1 };

	Inode();

	void setName(const std::string&);
	void setDataBlockLocation(int);
	void setNumberOfBlocks(int);
	void setSize(int);
	void setParent(int);
	void setType(Type);

	std::string getName() const;
	int getDataBlockLocation() const;
	int getNumberOfBlocks() const;
	int getSize() const;
	int getParent() const;
	Type getType() const;

	bool hasValidName() const; // name is null-terminated within its buffer
};
