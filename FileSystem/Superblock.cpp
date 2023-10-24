#include"Superblock.h"
#include <string.h>

Superblock::Superblock()
{
	for (int i = 0; i < TOTAL_BLOCKS; i++)
		Superblock::blockBitMap[i] = false;
	for (int i = 0; i < INODE_MAX; i++)
		Superblock::inodeBitMap[i] = false;
	freeBlocks = TOTAL_BLOCKS;
	freeSpace = TOTAL_BLOCKS * BLOCK_SIZE;
	inodeNum = 0;
}

void Superblock::operator=(const Superblock& other)
{
	freeBlocks = other.freeBlocks;
	freeSpace = other.freeSpace;
	inodeNum = other.inodeNum;
	for (int i = 0; i < TOTAL_BLOCKS; i++)
		blockBitMap[i] = other.blockBitMap[i];
	for (int i = 0; i < INODE_MAX; i++)
		inodeBitMap[i] = other.inodeBitMap[i];
}

void Superblock::SetBlockBitMap(int i, bool t)
{
	this->blockBitMap[i] = t;
}

void Superblock::SetInodeBitMap(int i, bool t)
{
	this->inodeBitMap[i] = t;
}

void Superblock::SetFreeBlocks(int i)
{
	this->freeBlocks = i;
}

void Superblock::SetFreeSpace(int i)
{
	this->freeSpace = i;
}

void Superblock::inodeNumUp()
{
	this->inodeNum++;
}

long Superblock::getFreeSpace()
{
	return freeSpace;
}

int Superblock::getFreeBlocks()
{
	return freeBlocks;
}

int Superblock::getInodeNum()
{
	return inodeNum;
}

bool* Superblock::getBlockBitMap()
{
	return blockBitMap;
}

bool* Superblock::getInodeBitMap()
{
	return inodeBitMap;
}

void Superblock::inodeNumDown()
{
	this->inodeNum--;
}
