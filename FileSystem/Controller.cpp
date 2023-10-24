#include "Controller.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <type_traits>

namespace fs = std::filesystem;

namespace {

const int NOT_FOUND = -1;
const int DEFAULT_FILE_BLOCKS = 5;
const int DISK_BYTES = TOTAL_BLOCKS * BLOCK_SIZE;
const char MAGIC[8] = { 'F', 'S', 'I', 'M', 'G', '0', '0', '1' };

static_assert(std::is_trivially_copyable<Inode>::value, "Inode is written to disk as raw bytes");
static_assert(std::is_trivially_copyable<Superblock>::value, "Superblock is written to disk as raw bytes");
static_assert(std::is_trivially_copyable<DiskImage>::value, "DiskImage is written to disk as raw bytes");

DiskImage formatted()
{
	DiskImage img{};
	std::memcpy(img.magic, MAGIC, sizeof MAGIC);
	img.inodes[0].setName("root");
	img.cwd = 0;
	return img;
}

void validate(const DiskImage& img)
{
	const auto corrupted = []() { throw std::runtime_error("The disk file is corrupted. Delete it to create a new disk."); };

	if (std::memcmp(img.magic, MAGIC, sizeof MAGIC) != 0) corrupted();
	if (!img.superblock.isInodeUsed(0)) corrupted();
	if (img.cwd < 0 || img.cwd >= INODE_MAX || !img.superblock.isInodeUsed(img.cwd)) corrupted();
	if (img.inodes[img.cwd].getType() != Inode::Folder) corrupted();

	for (int i = 0; i < INODE_MAX; i++)
	{
		if (!img.superblock.isInodeUsed(i)) continue;
		const Inode& n = img.inodes[i];
		const int parent = n.getParent();
		if (!n.hasValidName()) corrupted();
		if (n.getDataBlockLocation() < 0 || n.getNumberOfBlocks() < 0 ||
			n.getDataBlockLocation() + n.getNumberOfBlocks() > TOTAL_BLOCKS) corrupted();
		if (n.getSize() < 0 || n.getSize() > n.getNumberOfBlocks() * BLOCK_SIZE) corrupted();
		if (i == 0) continue;
		if (parent < 0 || parent >= INODE_MAX || !img.superblock.isInodeUsed(parent) ||
			img.inodes[parent].getType() != Inode::Folder) corrupted();
	}
}

DiskImage load(const std::string& path)
{
	std::ifstream in(path, std::ios::binary);
	DiskImage img{};
	in.read(reinterpret_cast<char*>(&img), sizeof img);
	if (!in || in.peek() != std::ifstream::traits_type::eof())
		throw std::runtime_error("The disk file has an invalid format. Delete it to create a new disk.");
	validate(img);
	return img;
}

void save(const std::string& path, const DiskImage& img)
{
	const std::string temp = path + ".tmp";
	{
		std::ofstream out(temp, std::ios::binary | std::ios::trunc);
		out.write(reinterpret_cast<const char*>(&img), sizeof img);
		out.flush();
		if (!out) throw std::runtime_error("Cannot write the disk file.");
	}
	std::error_code ec;
	fs::rename(temp, path, ec);
	if (ec) throw std::runtime_error("Cannot write the disk file: " + ec.message());
}

std::vector<std::string> split(const std::string& str, char separator)
{
	std::vector<std::string> parts;
	std::string current;
	for (char c : str)
	{
		if (c == separator)
		{
			if (!current.empty()) parts.push_back(current);
			current.clear();
		}
		else current += c;
	}
	if (!current.empty()) parts.push_back(current);
	return parts;
}

int blocksFor(size_t bytes)
{
	return static_cast<int>((bytes + BLOCK_SIZE - 1) / BLOCK_SIZE);
}

void validateName(const std::string& name)
{
	if (name.empty()) throw std::runtime_error("Name can't be empty.");
	if (name.size() > static_cast<size_t>(NAME_MAX_LENGTH))
		throw std::runtime_error("Name is too long (max " + std::to_string(NAME_MAX_LENGTH) + " characters).");
	if (name.find('/') != std::string::npos) throw std::runtime_error("Name can't contain '/'.");
	if (name == "." || name == ".." || name == "root") throw std::runtime_error("'" + name + "' is a reserved name.");
}

int findChild(const DiskImage& w, int parent, const std::string& name)
{
	for (int i = 1; i < INODE_MAX; i++)
		if (w.superblock.isInodeUsed(i) && w.inodes[i].getParent() == parent && w.inodes[i].getName() == name)
			return i;
	return NOT_FOUND;
}

std::string pathOf(const DiskImage& w, int dir)
{
	return dir == 0 ? "root" : "root/" + w.inodes[dir].getName();
}

int stepInto(const DiskImage& w, int dir, const std::string& token)
{
	if (token == ".") return dir;
	if (token == "..") return w.inodes[dir].getParent() < 0 ? dir : w.inodes[dir].getParent();
	const int child = findChild(w, dir, token);
	if (child == NOT_FOUND || w.inodes[child].getType() != Inode::Folder)
		throw std::runtime_error("Folder '" + token + "' doesn't exist.");
	return child;
}

// `name` is empty when the path denotes the folder `dir` itself.
struct ParsedPath
{
	int dir;
	std::string name;
};

// Accepts: name, folder/name, root/name, /name, .., root
ParsedPath parsePath(const DiskImage& w, const std::string& path)
{
	if (path.empty()) throw std::runtime_error("Path can't be empty.");

	std::vector<std::string> tokens = split(path, '/');
	bool absolute = path[0] == '/';
	if (!tokens.empty() && tokens[0] == "root")
	{
		absolute = true;
		tokens.erase(tokens.begin());
	}

	int dir = absolute ? 0 : w.cwd;
	if (tokens.empty()) return { dir, "" };

	for (size_t i = 0; i + 1 < tokens.size(); i++)
		dir = stepInto(w, dir, tokens[i]);

	const std::string& last = tokens.back();
	if (last == "." || last == "..") return { stepInto(w, dir, last), "" };
	return { dir, last };
}

int resolve(const DiskImage& w, const std::string& path)
{
	const ParsedPath p = parsePath(w, path);
	if (p.name.empty()) return p.dir;
	const int id = findChild(w, p.dir, p.name);
	if (id == NOT_FOUND) throw std::runtime_error("'" + path + "' doesn't exist.");
	return id;
}

int requireFile(const DiskImage& w, int id)
{
	if (w.inodes[id].getType() != Inode::File)
		throw std::runtime_error("'" + w.inodes[id].getName() + "' is a folder.");
	return id;
}

// Packs all data blocks to the start of the disk so free space becomes contiguous.
void defragment(DiskImage& w)
{
	std::vector<int> ids;
	for (int i = 1; i < INODE_MAX; i++)
		if (w.superblock.isInodeUsed(i) && w.inodes[i].getNumberOfBlocks() > 0)
			ids.push_back(i);
	std::sort(ids.begin(), ids.end(), [&w](int a, int b) {
		return w.inodes[a].getDataBlockLocation() < w.inodes[b].getDataBlockLocation();
	});

	w.superblock.setBlocks(1, TOTAL_BLOCKS - 1, false);
	int next = 1;
	for (int id : ids)
	{
		Inode& n = w.inodes[id];
		if (n.getDataBlockLocation() != next)
			std::memmove(w.data + next * BLOCK_SIZE, w.data + n.getDataBlockLocation() * BLOCK_SIZE,
				static_cast<size_t>(n.getNumberOfBlocks()) * BLOCK_SIZE);
		n.setDataBlockLocation(next);
		w.superblock.setBlocks(next, n.getNumberOfBlocks(), true);
		next += n.getNumberOfBlocks();
	}
}

int allocBlocks(DiskImage& w, int count)
{
	int first = w.superblock.findFreeRun(count);
	if (first == NOT_FOUND && w.superblock.getFreeBlocks() >= count)
	{
		defragment(w);
		first = w.superblock.findFreeRun(count);
	}
	if (first == NOT_FOUND) throw std::runtime_error("There is not enough space.");

	w.superblock.setBlocks(first, count, true);
	std::memset(w.data + first * BLOCK_SIZE, 0, static_cast<size_t>(count) * BLOCK_SIZE);
	return first;
}

int createEntry(DiskImage& w, int parent, const std::string& name, Inode::Type type, int blocks)
{
	validateName(name);
	if (findChild(w, parent, name) != NOT_FOUND)
		throw std::runtime_error("'" + name + "' already exists in this folder.");

	const int id = w.superblock.findFreeInode();
	if (id == NOT_FOUND) throw std::runtime_error("There is not enough space (no free i-nodes).");
	const int first = allocBlocks(w, blocks);

	Inode node;
	node.setName(name);
	node.setParent(parent);
	node.setType(type);
	node.setDataBlockLocation(first);
	node.setNumberOfBlocks(blocks);
	w.inodes[id] = node;
	w.superblock.setInodeUsed(id, true);
	return id;
}

void removeEntry(DiskImage& w, int id)
{
	if (id == 0) throw std::runtime_error("The root folder can't be removed.");

	if (w.inodes[id].getType() == Inode::Folder)
		for (int i = 1; i < INODE_MAX; i++)
			if (w.superblock.isInodeUsed(i) && w.inodes[i].getParent() == id)
				removeEntry(w, i);

	const Inode& n = w.inodes[id];
	w.superblock.setBlocks(n.getDataBlockLocation(), n.getNumberOfBlocks(), false);
	w.inodes[id] = Inode();
	w.superblock.setInodeUsed(id, false);
	if (w.cwd == id) w.cwd = 0;
}

std::string readData(const DiskImage& w, int id)
{
	const Inode& n = w.inodes[id];
	return std::string(w.data + n.getDataBlockLocation() * BLOCK_SIZE, n.getSize());
}

// Replaces the file content; reallocates (and defragments if needed) when it doesn't fit.
void writeData(DiskImage& w, int id, const std::string& bytes)
{
	if (bytes.size() > static_cast<size_t>(DISK_BYTES)) throw std::runtime_error("There is not enough space.");

	Inode& n = w.inodes[id];
	if (bytes.size() > static_cast<size_t>(n.getNumberOfBlocks()) * BLOCK_SIZE)
	{
		const int needed = blocksFor(bytes.size());
		w.superblock.setBlocks(n.getDataBlockLocation(), n.getNumberOfBlocks(), false);
		n.setNumberOfBlocks(0);
		n.setDataBlockLocation(0);
		n.setDataBlockLocation(allocBlocks(w, needed));
		n.setNumberOfBlocks(needed);
	}

	char* target = w.data + n.getDataBlockLocation() * BLOCK_SIZE;
	std::memset(target, 0, static_cast<size_t>(n.getNumberOfBlocks()) * BLOCK_SIZE);
	std::memcpy(target, bytes.data(), bytes.size());
	n.setSize(static_cast<int>(bytes.size()));
}

void transfer(DiskImage& w, const std::string& source, const std::string& destination, bool move)
{
	const int src = resolve(w, source);
	if (src == 0 || w.inodes[src].getType() == Inode::Folder)
		throw std::runtime_error("Folders can't be copied or moved.");
	const std::string srcName = w.inodes[src].getName();

	const ParsedPath d = parsePath(w, destination);
	int dir = d.dir;
	std::string name = d.name.empty() ? srcName : d.name;
	int existing = findChild(w, dir, name);

	// Destination is a folder: put the file into it under its own name.
	if (!d.name.empty() && existing != NOT_FOUND && w.inodes[existing].getType() == Inode::Folder)
	{
		dir = existing;
		name = srcName;
		existing = findChild(w, dir, name);
	}

	if (existing == src) throw std::runtime_error("Source and destination are the same file.");
	if (existing != NOT_FOUND) requireFile(w, existing);

	if (move && existing == NOT_FOUND)
	{
		validateName(name);
		w.inodes[src].setName(name);
		w.inodes[src].setParent(dir);
		return;
	}

	const std::string data = readData(w, src);
	const int target = existing != NOT_FOUND ? existing : createEntry(w, dir, name, Inode::File, DEFAULT_FILE_BLOCKS);
	writeData(w, target, data);
	if (move) removeEntry(w, src);
}

} // namespace

Controller::Controller(const std::string& diskPath) : diskPath(diskPath)
{
	if (fs::exists(diskPath))
	{
		image = load(diskPath);
	}
	else
	{
		image = formatted();
		save(diskPath, image);
	}
}

void Controller::commit(const DiskImage& w)
{
	save(diskPath, w);
	image = w;
}

void Controller::mkdir(const std::string& name)
{
	DiskImage w = image;
	if (w.cwd != 0) throw std::runtime_error("Maximum depth is 2.");
	createEntry(w, 0, name, Inode::Folder, 1);
	commit(w);
}

void Controller::create(const std::string& name)
{
	DiskImage w = image;
	createEntry(w, w.cwd, name, Inode::File, DEFAULT_FILE_BLOCKS);
	commit(w);
}

std::string Controller::cd(const std::string& path)
{
	DiskImage w = image;
	const int id = resolve(w, path);
	if (w.inodes[id].getType() != Inode::Folder) throw std::runtime_error("A file can't be the current directory.");
	w.cwd = id;
	commit(w);
	return pathOf(w, id);
}

std::vector<std::string> Controller::ls() const
{
	std::vector<std::string> names;
	for (int i = 1; i < INODE_MAX; i++)
		if (image.superblock.isInodeUsed(i) && image.inodes[i].getParent() == image.cwd)
			names.push_back(image.inodes[i].getName());
	return names;
}

StatInfo Controller::stat(const std::string& path) const
{
	const int id = resolve(image, path);
	const Inode& n = image.inodes[id];
	return { n.getName(), id, n.getDataBlockLocation(), n.getType(), n.getNumberOfBlocks(), n.getSize(),
		n.getParent() < 0 ? "-" : pathOf(image, n.getParent()) };
}

void Controller::echo(const std::string& path, const std::string& text)
{
	DiskImage w = image;
	writeData(w, requireFile(w, resolve(w, path)), text);
	commit(w);
}

std::string Controller::cat(const std::string& path) const
{
	return readData(image, requireFile(image, resolve(image, path)));
}

void Controller::rm(const std::string& path)
{
	DiskImage w = image;
	removeEntry(w, resolve(w, path));
	commit(w);
}

void Controller::rename(const std::string& path, const std::string& newName)
{
	DiskImage w = image;
	const int id = resolve(w, path);
	if (id == 0) throw std::runtime_error("The root folder can't be renamed.");
	validateName(newName);
	const int existing = findChild(w, w.inodes[id].getParent(), newName);
	if (existing != NOT_FOUND && existing != id)
		throw std::runtime_error("'" + newName + "' already exists in this folder.");
	w.inodes[id].setName(newName);
	commit(w);
}

void Controller::cp(const std::string& source, const std::string& destination)
{
	DiskImage w = image;
	transfer(w, source, destination, false);
	commit(w);
}

void Controller::mv(const std::string& source, const std::string& destination)
{
	DiskImage w = image;
	transfer(w, source, destination, true);
	commit(w);
}

void Controller::get(const std::string& localPath)
{
	std::ifstream in(localPath, std::ios::binary);
	if (!in) throw std::runtime_error("Cannot open the local file '" + localPath + "'.");
	in.seekg(0, std::ios::end);
	const std::streamoff size = in.tellg();
	if (size < 0) throw std::runtime_error("Cannot read the local file '" + localPath + "'.");
	if (size > DISK_BYTES) throw std::runtime_error("There is not enough space.");
	in.seekg(0, std::ios::beg);
	const std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());

	DiskImage w = image;
	const int id = createEntry(w, w.cwd, fs::path(localPath).filename().string(), Inode::File, DEFAULT_FILE_BLOCKS);
	writeData(w, id, data);
	commit(w);
}

void Controller::put(const std::string& path)
{
	const int id = requireFile(image, resolve(image, path));
	const std::string data = readData(image, id);

	std::ofstream out(image.inodes[id].getName(), std::ios::binary | std::ios::trunc);
	out.write(data.data(), static_cast<std::streamsize>(data.size()));
	out.flush();
	if (!out) throw std::runtime_error("Cannot write the local file '" + image.inodes[id].getName() + "'.");
}

std::string Controller::currentPath() const
{
	return pathOf(image, image.cwd);
}
