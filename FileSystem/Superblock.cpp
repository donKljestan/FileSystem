#include "Superblock.h"
#include <cstring>
#include <stdexcept>

Superblock::Superblock()
{
	std::memset(inodeBitMap, 0, sizeof inodeBitMap);
	std::memset(blockBitMap, 0, sizeof blockBitMap);
	inodeBitMap[0] = true;
	blockBitMap[0] = true;
}

int Superblock::getFreeBlocks() const
{
	int free = 0;
	for (int i = 0; i < TOTAL_BLOCKS; i++)
		if (!blockBitMap[i])
			free++;
	return free;
}

long Superblock::getFreeSpace() const
{
	return static_cast<long>(getFreeBlocks()) * BLOCK_SIZE;
}

int Superblock::getInodeNum() const
{
	int used = 0;
	for (int i = 0; i < INODE_MAX; i++)
		if (inodeBitMap[i])
			used++;
	return used;
}

bool Superblock::isInodeUsed(int i) const
{
	if (i < 0 || i >= INODE_MAX) throw std::out_of_range("Inode index out of range.");
	return inodeBitMap[i];
}

void Superblock::setInodeUsed(int i, bool used)
{
	if (i < 0 || i >= INODE_MAX) throw std::out_of_range("Inode index out of range.");
	inodeBitMap[i] = used;
}

int Superblock::findFreeInode() const
{
	for (int i = 1; i < INODE_MAX; i++)
		if (!inodeBitMap[i])
			return i;
	return -1;
}

bool Superblock::isBlockUsed(int i) const
{
	if (i < 0 || i >= TOTAL_BLOCKS) throw std::out_of_range("Block index out of range.");
	return blockBitMap[i];
}

int Superblock::findFreeRun(int count) const
{
	int runStart = -1;
	int runLength = 0;
	for (int i = 1; i < TOTAL_BLOCKS; i++)
	{
		if (blockBitMap[i])
		{
			runLength = 0;
			continue;
		}
		if (runLength == 0) runStart = i;
		if (++runLength == count) return runStart;
	}
	return -1;
}

void Superblock::setBlocks(int first, int count, bool used)
{
	if (first < 0 || count < 0 || first + count > TOTAL_BLOCKS) throw std::out_of_range("Block range out of range.");
	for (int i = first; i < first + count; i++)
		blockBitMap[i] = used;
}
