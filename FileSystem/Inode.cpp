#include "Inode.h"
#include <cstring>

Inode::Inode() : dataBlockLocation(0), numberOfBlocks(0), size(0), parent(-1), type(Folder)
{
	std::memset(name, 0, sizeof name);
}

void Inode::setName(const std::string& str)
{
	std::memset(name, 0, sizeof name);
	std::strncpy(name, str.c_str(), NAME_MAX_LENGTH);
}

void Inode::setDataBlockLocation(int i) { dataBlockLocation = i; }
void Inode::setNumberOfBlocks(int i) { numberOfBlocks = i; }
void Inode::setSize(int i) { size = i; }
void Inode::setParent(int i) { parent = i; }
void Inode::setType(Type t) { type = t; }

std::string Inode::getName() const { return std::string(name); }
int Inode::getDataBlockLocation() const { return dataBlockLocation; }
int Inode::getNumberOfBlocks() const { return numberOfBlocks; }
int Inode::getSize() const { return size; }
int Inode::getParent() const { return parent; }
Inode::Type Inode::getType() const { return type == File ? File : Folder; }

bool Inode::hasValidName() const
{
	return std::memchr(name, '\0', sizeof name) != nullptr;
}
