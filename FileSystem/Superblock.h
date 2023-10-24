#pragma once

// Deliberately tiny values: the whole disk image stays a few KB and is easy to test.
const int BLOCK_SIZE = 20;
const int TOTAL_BLOCKS = 100;
const int INODE_MAX = 20;

// Allocation state of the disk. Inode 0 (root) and block 0 are reserved.
class Superblock
{
	bool inodeBitMap[INODE_MAX];
	bool blockBitMap[TOTAL_BLOCKS];

public:
	Superblock();

	int getFreeBlocks() const;
	long getFreeSpace() const;
	int getInodeNum() const;

	bool isInodeUsed(int) const;
	void setInodeUsed(int, bool);
	int findFreeInode() const; // -1 if there is none

	bool isBlockUsed(int) const;
	int findFreeRun(int count) const; // first block of `count` contiguous free blocks, -1 if there is none
	void setBlocks(int first, int count, bool used);
};
