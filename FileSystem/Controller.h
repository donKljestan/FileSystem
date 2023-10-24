#pragma once
#include "Inode.h"
#include "Superblock.h"

const int SUPERBLOCK_SIZE = sizeof(Superblock);
const int INODE_SIZE = sizeof(Inode);
const int FIRST_BLOCK_LOCATION = SUPERBLOCK_SIZE + INODE_MAX * INODE_SIZE + 10; //prvi blok za upis, u bajtovima

//Stalno cuvaj informaciju o centralnom direktorijumu

class Controller {

	const std::string DISK_LOCATION = "./disk.bin"; // Optionally create option for adding disk, and path to it
	Superblock superblock;
	Inode inode[INODE_MAX];


public:

	Controller();

	void mkdir(const std::string&) noexcept(false);
	void create(const std::string&) noexcept(false);
	void cd(const std::string&) noexcept(false);
	void CD();//cd..
	void put(const std::string&) noexcept(false);
	void get(const std::string&) noexcept(false);
	void ls() noexcept(false);
	void cp(const std::string&, const std::string&) noexcept(false);
	void mv(const std::string&, const std::string&) noexcept(false);
	void rename(const std::string&, const std::string&) noexcept(false);
	void echo(const std::string&, const std::string&) noexcept(false);
	void cat(const std::string&) noexcept(false);
	void rm(const std::string&) noexcept(false);
	void stat(const std::string&) noexcept(false);

	//~Controller();

private:
	int checkFreeInode(Superblock &);
	int checkFreeBlocks(const int, Superblock&);
	int search(std::string, FILE*);
	//void Defragmentation();
	int findNodeId(const int, std::string, FILE*);
	void writeInode(const Inode&, const int, FILE *fp); //upisuje i cvor u datoteku
	Inode* loadInode(int, FILE*);
	int loadCentralDirectory(FILE *);
	Superblock* loadSuperblock(FILE*);
};