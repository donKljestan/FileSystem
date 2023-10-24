#pragma once
#include "Superblock.h"
#include <string>

class Inode
{
	std::string name; //name of file/folder
	int DATA_BLOCK_LOCATION; //redni broj(adresa) bloka kojem odgovara ovaj i-cvor
	int numberOfBlocks; //velicina fajla
	int parent;
	int type; //0 ako je folder, 1 ako je datoteka

public:
	Inode();
	void SetName(std::string);
	void SetDataBlockLocation(int);
	void SetNumberOfBlocks(int);
	void SetParent(int);
	void SetType(int);
	int getParent();
	int getNumberOfBlocks();
	int getDataBlockLocation();
	int getType();
	const std::string& getName() const; 
};