#define _CRT_SECURE_NO_DEPRECATE // Enabling fread and fwrite functions to compile
#include "Controller.h"
#include <vector>
#include <stdio.h>
#include <regex>
#include <string.h>

Controller::Controller()
{

	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "w");
	superblock = Superblock();
	Superblock s[] = { superblock };
	int CentralDirectory = 0;

	rewind(diskPtr);
	fwrite(s, SUPERBLOCK_SIZE, 1, diskPtr); //upis superbloka
	fseek(diskPtr, 1, SEEK_CUR); //pomeraj od jednog bajta
	fwrite(&CentralDirectory, sizeof(int), 1, diskPtr); //upis cd
	fseek(diskPtr, 1, SEEK_CUR);//pomeraj od jednog bajta
	fwrite(&inode, INODE_SIZE, INODE_MAX, diskPtr); //upis niza I-cvorova
	fclose(diskPtr);
}
void Controller::mkdir(const std::string& str) noexcept(false)
{
	
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	if (CentralDirectory != NULL) throw std::exception("Maximum depth is 2.\n");

	Superblock *s = loadSuperblock(diskPtr);

	int id = checkFreeInode(*s); //od ovog mesta uzimaju se i-cvorovi (prvi je 1)

	Inode* inode = loadInode(id, diskPtr);
	int FreeBlocks = checkFreeBlocks(1, *s); //od ovog mesta pocinje da se upisuje fajl (prvi je 1)
	inode->SetDataBlockLocation(FreeBlocks);

	if (FreeBlocks == 0 || id == 0) throw std::exception("There is not enough space.\n");//
	s->inodeNumUp();
	s->SetFreeBlocks(s->getFreeBlocks() - 1);//za folder ostavljam jedan blok

	if (findNodeId(0, str, diskPtr) != 0) throw std::exception("Folder already exsist.\n");
	inode->SetNumberOfBlocks(1);
	inode->SetName(str);


	inode->SetParent(0);
	inode->SetType(0);

	s->SetInodeBitMap(id, true);
	//kreirati folder
	//upload superbloka i inoda
	rewind(diskPtr);
	fwrite(s, SUPERBLOCK_SIZE, 1, diskPtr);
	writeInode(*inode, id, diskPtr);
	fclose(diskPtr);
}
void Controller::create(const std::string& str) noexcept(false)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	int id = checkFreeInode(*s);

	Inode* inode = loadInode(id, diskPtr);
	int FreeBlocks = checkFreeBlocks(5, *s);
	inode->SetDataBlockLocation(FreeBlocks);

	if (FreeBlocks == 0 || id == 0) throw std::exception("There is not enough space.\n");
	s->inodeNumUp();
	s->SetFreeBlocks(s->getFreeBlocks() - 5);
	inode->SetParent(CentralDirectory);
	if (findNodeId(CentralDirectory, str, diskPtr) != 0) throw std::exception("File already exsist in this folder.\n");
	inode->SetNumberOfBlocks(5);
	inode->SetName(str);
	inode->SetType(1);

	s->SetInodeBitMap(id, true);
	//upload superbloka i inoda
	rewind(diskPtr);
	fwrite(s, SUPERBLOCK_SIZE, 1, diskPtr);
	writeInode(*inode, id, diskPtr);
	fclose(diskPtr);
}

void Controller::get(const std::string& str) noexcept(false) //uzima sa racunara i upisuje u moju dat
{
	FILE* fp = fopen(str.c_str(), "r+");
	create(str);
	char c;
	
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	int id = findNodeId(CentralDirectory, str, diskPtr);
	Inode* inode = loadInode(id, diskPtr);

	int position = FIRST_BLOCK_LOCATION + (inode->getDataBlockLocation())*BLOCK_SIZE;
	fseek(diskPtr, position, SEEK_SET);

	while ((c = fgetc(fp)) != EOF)
	{
		fputc(c, diskPtr);
	}

	
	fclose(diskPtr);
	fclose(fp);
}

void Controller::put(const std::string& str) noexcept(false) //uzima iz moje datoteke i upisuje na racunar
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	int id = findNodeId(CentralDirectory, str, diskPtr);

	if (id == false) throw std::exception("This file doesn't exist.");

	Inode* inode = loadInode(id, diskPtr);

	int position = FIRST_BLOCK_LOCATION + (inode->getDataBlockLocation())*BLOCK_SIZE;
	fseek(diskPtr, position, SEEK_SET);

	char c;
	FILE *fp = fopen(str.c_str(), "w+");
		

	for (int i = 0; i < (inode->getNumberOfBlocks())*BLOCK_SIZE; i++)
	{
		c = fgetc(diskPtr);
		fputc(c, fp);
	}
	
	fclose(diskPtr);
	fclose(fp);
}
void Controller::echo(const std::string& name, const std::string& str) noexcept(false)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);
	Superblock *s = loadSuperblock(diskPtr);

	int id = findNodeId(CentralDirectory, name, diskPtr);

	if (id == false) throw std::exception("This file doesn't exist.\n");
	

	Inode* inode = loadInode(id, diskPtr);
	int n = str.length();
	if (n < 5 * BLOCK_SIZE) //ako moze stati u postojecu datoteku sa inodom 'inode'
	{
		int position = FIRST_BLOCK_LOCATION + (inode->getDataBlockLocation())*BLOCK_SIZE;
		fseek(diskPtr, position, SEEK_SET);


		char c = '0';
		for (int i = 0; i < n; i++)
			fwrite(&str[i], 1, 1, diskPtr);
		fwrite(&c, 1, 1, diskPtr);
		fclose(diskPtr);
	} 
	/*Ako je string veci nego sto moze stati u datoteku, izbrisacu postojecu a napraviti novu u koju moze stati.*/
	else if (n > 5 * BLOCK_SIZE)
	{
		//brisanje postojece
		s->SetInodeBitMap(id, false);
		s->inodeNumDown();

		int blocksAllocated = inode->getNumberOfBlocks();
		int DataBlockLocation = inode->getDataBlockLocation();

		s->SetFreeBlocks(s->getFreeBlocks() + blocksAllocated);
		for (int i = DataBlockLocation; i < DataBlockLocation + blocksAllocated; i++)
			s->SetBlockBitMap(i, false);
		inode = new Inode();
		writeInode(*inode, id, diskPtr);

		//kreiranje nove
		int i = checkFreeInode(*s);
		int p=(n/BLOCK_SIZE)+2;
		Inode* inode = loadInode(i, diskPtr);
		int FreeBlocks = checkFreeBlocks(p, *s);
		inode->SetDataBlockLocation(FreeBlocks);

		if (FreeBlocks == 0 || i == 0) throw std::exception("There is not enough space.\n");
		s->inodeNumUp();
		s->SetFreeBlocks(s->getFreeBlocks() - p);
		inode->SetParent(CentralDirectory);
		if (findNodeId(CentralDirectory, name, diskPtr) != 0) throw std::exception("File already exsist in this folder.\n");
		inode->SetNumberOfBlocks(p);
		inode->SetName(name);
		inode->SetType(1);
		s->SetInodeBitMap(i, true);

		//upis stringa u fajl

		int position = FIRST_BLOCK_LOCATION + (inode->getDataBlockLocation())*BLOCK_SIZE;
		fseek(diskPtr, position, SEEK_SET);


		char c = '0';
		for (int i = 0; i < n; i++)
			fwrite(&str[i], 1, 1, diskPtr);
		fwrite(&c, 1, 1, diskPtr);

		//upload superbloka i inoda
		rewind(diskPtr);
		fwrite(s, SUPERBLOCK_SIZE, 1, diskPtr);
		writeInode(*inode, i, diskPtr);
		fclose(diskPtr);
	}
}
void Controller::rename(const std::string& name, const std::string& str)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);
	Superblock *s = loadSuperblock(diskPtr);
	int id = findNodeId(CentralDirectory, name, diskPtr);
	if (id == false) throw std::exception("This file doesn't exist.\n");
	Inode* inode = loadInode(id, diskPtr);
	inode->SetName(str);
	writeInode(*inode, id, diskPtr);
	fclose(diskPtr);
}
void Controller::stat(const std::string& str) noexcept(false)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	int id = findNodeId(CentralDirectory, str, diskPtr);

	if (id == false) throw std::exception("This file doesn't exist.\n");
	Inode* inode = loadInode(id, diskPtr);

	std::cout << "Ime: " << inode->getName() << std::endl;
	std::cout << "Redni broj i-cvora: " << id << std::endl;
	std::cout << "Redni broj bloka: " << inode->getDataBlockLocation() << std::endl;
	std::cout << "Tip: ";
	if (inode->getType() == 0) std::cout << "folder" << std::endl;
	else std::cout << "fajl" << std::endl;
	std::cout << "Broj blokova: " << inode->getNumberOfBlocks() << std::endl;
	std::cout << "Vrhovni direktorijum: ";
	if (CentralDirectory == 0) std::cout << "ROOT" << std::endl;
	else
	{
		Inode *i = loadInode(CentralDirectory, diskPtr);
		std::cout << i->getName() << std::endl;
	}
	fclose(diskPtr);
}

void Controller::ls() noexcept(false)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);
	Superblock *s = loadSuperblock(diskPtr);
	Inode *inode[INODE_MAX];

	for (int i = 0; i < INODE_MAX; i++)
	{
		inode[i] = loadInode(i, diskPtr);
		if (inode[i]->getParent() == CentralDirectory)
			std::cout << inode[i]->getName() << std::endl;
	}

}

void Controller::cd(const std::string& str) noexcept(false)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	Inode* inode[INODE_MAX];
	int CD;
	int i;
	for (i = 0; i < INODE_MAX; i++)
	{
		inode[i] = loadInode(i, diskPtr);
		if (inode[i]->getName() == str)
		{
			CD = i; i = INODE_MAX + 1; //za izlaz iz petlje
		}
	}

	if (i == INODE_MAX) throw std::exception("Folder doesn't exsist.\n");
	if (inode[CD]->getType() == 1) throw std::exception("File can not be the central directory.\n");

	std::cout << "Centralni direktorijum: " << inode[CD]->getName() << std::endl;

	rewind(diskPtr);
	fseek(diskPtr, SUPERBLOCK_SIZE + 1, SEEK_SET);
	fwrite(&CD, sizeof(int), 1, diskPtr);
	fclose(diskPtr);

}

void Controller::CD()
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = 0;
	rewind(diskPtr);
	fseek(diskPtr, SUPERBLOCK_SIZE + 1, SEEK_SET);
	fwrite(&CentralDirectory, sizeof(int), 1, diskPtr);
	std::cout << "Centralni direktorijum je root.\n" << std::endl;
	fclose(diskPtr);
}

void Controller::cat(const std::string& str) noexcept(false)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	int id = findNodeId(CentralDirectory, str, diskPtr);

	if (id == false) throw std::exception("This file doesn't exist.");

	Inode* inode = loadInode(id, diskPtr);

	int position = FIRST_BLOCK_LOCATION + (inode->getDataBlockLocation())*BLOCK_SIZE;
	fseek(diskPtr, position, SEEK_SET);

	char c = -1;
	int i = 0; 
	while (c != '0')
	{
		fread(&c, 1, 1, diskPtr);
		std::cout << c;
	}
	std::cout << std::endl;
	fclose(diskPtr);

}

void Controller::mv(const std::string& izvorisna, const std::string &odredisna) noexcept(false)
{
	//unos mora biti root/file ili folder/file

	cp(izvorisna, odredisna);

	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	//brisanje izvorisne
	int id = search(izvorisna, diskPtr);
	if (id == false) throw std::exception("Ne postoji izvorisna datoteka sa tim imenom.\n");

	Inode *inode = loadInode(id, diskPtr);
	s->SetInodeBitMap(id, false);
	s->inodeNumDown();

	int blocksAllocated = inode->getNumberOfBlocks();
	int DataBlockLocation = inode->getDataBlockLocation();


	s->SetFreeBlocks(s->getFreeBlocks() + blocksAllocated);

	for (int i = DataBlockLocation; i < DataBlockLocation + blocksAllocated; i++)
		s->SetBlockBitMap(i, false);
	inode = new Inode();
	writeInode(*inode, id, diskPtr);
	
	rewind(diskPtr);
	fwrite(s, SUPERBLOCK_SIZE, 1, diskPtr);
	fclose(diskPtr);
}

void Controller::cp(const std::string& izvorisna, const std::string &odredisna) noexcept(false)
{
	//unos mora biti //root/file ili folder/file
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	int id1 = search(izvorisna, diskPtr);
	int id2 = search(odredisna, diskPtr);
	if (id1 == false) throw std::exception("Ne postoji izvorisna datoteka sa tim imenom.\n");
	if (id2 == false) throw std::exception("Ne postoji odredisna datoteka sa tim imenom.\n");

	Inode* inode1 = loadInode(id1, diskPtr); //i-cvor izvorisne
	Inode* inode2 = loadInode(id2, diskPtr); //i-cvor odredisne

	//procitati bajt po bajt iz izvorisne pa upisati u odredisnu
	int position = FIRST_BLOCK_LOCATION + (inode1->getDataBlockLocation())*BLOCK_SIZE;
	fseek(diskPtr, position, SEEK_SET);
	
	
	//char niz [n]; //u ovaj niz ce se procitati sadrzaj datoteke
	//std::vector<char> niz;
	const int n = (inode1->getNumberOfBlocks())*BLOCK_SIZE;
	char niz[100];
	fread(&niz, sizeof(char), n, diskPtr); 
	
	//sada upisati u odredisnu datoteku
	position = FIRST_BLOCK_LOCATION + (inode2->getDataBlockLocation())*BLOCK_SIZE;
	fseek(diskPtr, position, SEEK_SET);
	fwrite(&niz, sizeof(char), n, diskPtr);
	fclose(diskPtr);

}

void Controller::rm(const std::string& str) noexcept(false)
{
	FILE *diskPtr = fopen(DISK_LOCATION.c_str(), "r+");
	int CentralDirectory = loadCentralDirectory(diskPtr);

	Superblock *s = loadSuperblock(diskPtr);

	int id = findNodeId(CentralDirectory, str, diskPtr);

	if (id == false) throw std::exception("This file doesn't exist.");

	Inode* inode = loadInode(id, diskPtr);

	if (inode->getType() == 0) //folder
	{
		//CentralniDirektorjium je redni broj i cvora
		for (int i = 0; i < INODE_MAX; i++) //trazim fajlove koji se nalaze u ovom folderu
		{
			Inode *iNode = loadInode(i, diskPtr);
			
			if (iNode->getParent() == id) //treba obrisati ovaj fajl sa ovim inodom
			{
				s->SetInodeBitMap(i, false);
				s->inodeNumDown();

				int blocksAllocated = iNode->getNumberOfBlocks();
				int DataBlockLocation = iNode->getDataBlockLocation();

				s->SetFreeBlocks(s->getFreeBlocks() + blocksAllocated);
				for (int j = DataBlockLocation; j < DataBlockLocation + blocksAllocated; j++)
					s->SetBlockBitMap(j, false);

				iNode = new Inode();
				writeInode(*iNode, i, diskPtr);

			}
			
		}
		//za folder
		s->SetInodeBitMap(id, false);
		s->inodeNumDown();

		s->SetFreeBlocks(s->getFreeBlocks() + inode->getNumberOfBlocks());
		s->SetBlockBitMap(inode->getDataBlockLocation(), false);

		inode = new Inode();
		writeInode(*inode, id, diskPtr);
	}
	else if (inode->getType() == 1) //file
	{
		
		s->SetInodeBitMap(id, false);
		s->inodeNumDown();

		int blocksAllocated = inode->getNumberOfBlocks();
		int DataBlockLocation = inode->getDataBlockLocation();
		

		s->SetFreeBlocks(s->getFreeBlocks() + blocksAllocated);
		
		for (int i = DataBlockLocation; i < DataBlockLocation + blocksAllocated; i++)
			s->SetBlockBitMap(i, false);
		inode = new Inode();
		writeInode(*inode, id, diskPtr);
	}
	else throw std::exception("Error.\n");


	//upload superbloka
	rewind(diskPtr);
	fwrite(s, SUPERBLOCK_SIZE, 1, diskPtr);
	fclose(diskPtr);

}

int Controller::checkFreeInode(Superblock& s)
{
	bool* inodeBitmap = s.getInodeBitMap();
	for (int i = 1; i < INODE_MAX; i++)
		if (inodeBitmap[i] == false)
			return i;
	return 0;
}
int Controller::checkFreeBlocks(const int blocksAllocated, Superblock& s)
{
	//n=blocksAllocated
	//pokusam naci uzastopnih n blokova
	//ako nema uzastopnih pokusam naci n blokova slucajno rasporedjenih
	//ako ima n blokova slucajno rasporedjenih, pozivam funkciju defragmentacija
	//ukoliko nema ni n slucajno rasporednjenih vracam false
	bool* blockBitMap = s.getBlockBitMap();
	if (blocksAllocated > ((64 * 1024) / BLOCK_SIZE)) throw std::exception("Max size is 64KB.\n");
	for (int i = 1; i < TOTAL_BLOCKS; ++i) {
		if (blockBitMap[i] == false)
		{
			int j;
			for (j = i + 1; j < i + blocksAllocated; j++) //provjeravamo da li postoji zaredom potreban br. slobodnih blokova
				if (blockBitMap[j] == true)
					break;
			if (j == i + blocksAllocated)
			{
				for (j = i; j < i + blocksAllocated; j++)
					s.SetBlockBitMap(j, true); 
				return i; //od ovog mesta pocinje da se upisuje fajl
				//nasao sam n uzastopnih blokova
			}
		}
	}
	int counter = 0;
	for (int i = 0; i < TOTAL_BLOCKS; i++)
		if (blockBitMap[i] == false)
			counter++;
	//if (counter >= blocksAllocated)
	//{
	//	Defragmentation();
	//	checkFreeBlocks(blocksAllocated, s);
	//}
	return 0; //ako nema dovoljno blokova vraca se 0
}

int Controller::findNodeId(const int parent, std::string name, FILE *diskPtr)
{
	Inode* inode[INODE_MAX];
	for (int i = 1; i < INODE_MAX; i++)
	{
		inode[i] = loadInode(i, diskPtr);

		const std::string s = inode[i]->getName();
		if (inode[i]->getParent() == parent && (s.compare(name)) == 0)
			return i;
	}					//ako postoji folder/file sa tim imenom
	return 0; //ne postoji
}

int Controller::search(std::string name, FILE *diskPtr)
{
	//ova funkcija ce pronaci datoteku na fajl sistemu i vratiti njen i cvor
	//unos mora biti root/file ili folder/file

	int k = 0;
	char c='0';
	for (; c != '/'; ++k)
		c = name[k]; 
	std::string directory = name.substr(0, k - 1);
	std::string str = name.substr(k, name.length());
	if (directory == "root")
	{
		return findNodeId(0, str, diskPtr);
	}
	else
	{
		int i = findNodeId(0, directory, diskPtr);
		return findNodeId(i, str, diskPtr);
	}
	
}

void Controller::writeInode(const Inode& node, const int id, FILE *fp)
{
	int position = SUPERBLOCK_SIZE + id * INODE_SIZE + 6;//pozicija u datoteci od pocetka u bajtovima
	fseek(fp, position, SEEK_SET); //premotavanje u datoteci na poziciju na koju zelim da upisem cvor
	fwrite(&node, sizeof(Inode), 1, fp);
}

Inode* Controller::loadInode(int inodeId, FILE* diskPtr) {

	int inodeLocation = SUPERBLOCK_SIZE + inodeId * INODE_SIZE + 6;
	Inode* inode = new Inode();

	fseek(diskPtr, inodeLocation, SEEK_SET);
	fread(inode, INODE_SIZE, 1, diskPtr);

	return inode;
}

int Controller::loadCentralDirectory(FILE* diskPtr)
{
	int CentralDirectory;
	int position = SUPERBLOCK_SIZE + 1;

	fseek(diskPtr, position, SEEK_SET);
	fread(&CentralDirectory, sizeof(int), 1, diskPtr);
	return CentralDirectory;
}

Superblock* Controller::loadSuperblock(FILE *diskPtr)
{
	Superblock* superblock = new Superblock();

	rewind(diskPtr);
	fread(superblock, SUPERBLOCK_SIZE, 1, diskPtr);

	return superblock;
}