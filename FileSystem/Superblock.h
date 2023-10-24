#pragma once
#include "Inode.h"
#include <string>
#include<iostream>

const int BLOCK_SIZE = 20; //4KB
const int TOTAL_BLOCKS = 100; //20MB/4KB
const int INODE_MAX = 20; //Maksimalan broj datoteka/foldera na disku.

class Superblock
{
	long freeSpace;
	int freeBlocks;
	int inodeNum;
	bool inodeBitMap[INODE_MAX];
	bool blockBitMap[TOTAL_BLOCKS];

public:
	const int ROOT_INODE_PTR = sizeof(Superblock);

	Superblock();

	void operator=(const Superblock&);

	long getFreeSpace();
	int getFreeBlocks();
	int getInodeNum();
	bool *getInodeBitMap();
	bool *getBlockBitMap();
	void SetBlockBitMap(int, bool);
	void SetInodeBitMap(int, bool);
	void SetFreeSpace(int);
	void SetFreeBlocks(int);
	void inodeNumUp();
	void inodeNumDown();
};


