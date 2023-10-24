#include "Inode.h"

Inode::Inode() :name("."), type(0)
{
	this->DATA_BLOCK_LOCATION = 0;
	this->numberOfBlocks = 0;
	this->parent = -1;
}

void Inode::SetName(std::string str)
{
	this->name = str;
}

void Inode::SetDataBlockLocation(int i)
{
	this->DATA_BLOCK_LOCATION = i;
}

void Inode::SetNumberOfBlocks(int i)
{
	this->numberOfBlocks = i;
}

void Inode::SetParent(int i)
{
	this->parent = i;
}

void Inode::SetType(int i)
{
	this->type = i;
}

int Inode::getDataBlockLocation()
{
	return DATA_BLOCK_LOCATION;
}

int Inode::getNumberOfBlocks()
{
	return numberOfBlocks;
}

int Inode::getType()
{
	return type;
}

int Inode::getParent()
{
	return parent;
}

const std::string& Inode::getName() const
{
	return name;
}