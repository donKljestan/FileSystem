#include "UI.h"
#include <iostream>
#include <stdexcept>

namespace {

const char* WHITESPACE = " \t\r\n";

std::string trim(const std::string& str)
{
	const size_t begin = str.find_first_not_of(WHITESPACE);
	if (begin == std::string::npos) return "";
	return str.substr(begin, str.find_last_not_of(WHITESPACE) - begin + 1);
}

// Splits "first rest of the line" into its first word and the trimmed remainder.
void splitFirst(const std::string& str, std::string& first, std::string& rest)
{
	const size_t end = str.find_first_of(WHITESPACE);
	first = str.substr(0, end);
	rest = end == std::string::npos ? "" : trim(str.substr(end));
}

[[noreturn]] void usage(const std::string& syntax)
{
	throw std::runtime_error("Usage: " + syntax);
}

void noArguments(const std::string& rest, const std::string& syntax)
{
	if (!rest.empty()) usage(syntax);
}

const std::string& oneArgument(const std::string& rest, const std::string& syntax)
{
	if (rest.empty()) usage(syntax);
	return rest;
}

void twoArguments(const std::string& rest, const std::string& syntax, std::string& first, std::string& second)
{
	splitFirst(rest, first, second);
	if (first.empty() || second.empty()) usage(syntax);
}

} // namespace

void UI::run()
{
	std::string input;

	while (true)
	{
		std::cout << controller.currentPath() << "> ";
		if (!std::getline(std::cin, input)) break;

		input = trim(input);
		if (input.empty()) continue;

		try
		{
			if (!execute(input)) break;
		}
		catch (const std::exception& e)
		{
			std::cout << e.what() << std::endl;
		}
	}
}

bool UI::execute(const std::string& line)
{
	std::string command, rest, first, second;
	splitFirst(line, command, rest);

	if (command == "exit")
	{
		noArguments(rest, "exit");
		return false;
	}
	else if (command == "help")
	{
		help();
	}
	else if (command == "mkdir")
	{
		controller.mkdir(oneArgument(rest, "mkdir <name>"));
	}
	else if (command == "create")
	{
		controller.create(oneArgument(rest, "create <name>"));
	}
	else if (command == "echo")
	{
		splitFirst(rest, first, second);
		if (first.empty() || second.empty()) usage("echo <file> <text>");
		controller.echo(first, second);
	}
	else if (command == "cat")
	{
		std::cout << controller.cat(oneArgument(rest, "cat <file>")) << std::endl;
	}
	else if (command == "cd")
	{
		const std::string path = controller.cd(oneArgument(rest, "cd <folder>"));
		std::cout << "Current directory: " << path << std::endl;
	}
	else if (command == "CD")
	{
		noArguments(rest, "CD");
		const std::string path = controller.cd("root");
		std::cout << "Current directory: " << path << std::endl;
	}
	else if (command == "ls")
	{
		noArguments(rest, "ls");
		for (const std::string& name : controller.ls())
			std::cout << name << std::endl;
	}
	else if (command == "stat")
	{
		const StatInfo info = controller.stat(oneArgument(rest, "stat <name>"));
		std::cout << "Name: " << info.name << std::endl;
		std::cout << "I-node number: " << info.inode << std::endl;
		std::cout << "Block number: " << info.block << std::endl;
		std::cout << "Type: " << (info.type == Inode::Folder ? "folder" : "file") << std::endl;
		std::cout << "Blocks: " << info.blocks << std::endl;
		std::cout << "Size: " << info.size << " B" << std::endl;
		std::cout << "Parent directory: " << info.parent << std::endl;
	}
	else if (command == "rm")
	{
		controller.rm(oneArgument(rest, "rm <name>"));
	}
	else if (command == "rename")
	{
		twoArguments(rest, "rename <name> <new name>", first, second);
		controller.rename(first, second);
	}
	else if (command == "cp")
	{
		twoArguments(rest, "cp <source> <destination>", first, second);
		controller.cp(first, second);
	}
	else if (command == "mv")
	{
		twoArguments(rest, "mv <source> <destination>", first, second);
		controller.mv(first, second);
	}
	else if (command == "get")
	{
		controller.get(oneArgument(rest, "get <local file>"));
	}
	else if (command == "put")
	{
		controller.put(oneArgument(rest, "put <file>"));
	}
	else
	{
		throw std::runtime_error("Unknown command '" + command + "'. Type 'help' for the list of commands.");
	}
	return true;
}

void UI::help() const
{
	std::cout <<
		"mkdir <name>              create a folder (root only)\n"
		"create <name>             create an empty file\n"
		"cd <folder>               change folder (also: cd .., cd root); CD goes to root\n"
		"ls                        list the current folder\n"
		"stat <name>               show i-node information\n"
		"echo <file> <text>        write text to a file\n"
		"cat <file>                print a file\n"
		"rename <name> <new name>  rename a file or folder\n"
		"rm <name>                 remove a file or a folder with its content\n"
		"cp <source> <dest>        copy a file (dest may be a new name or a folder)\n"
		"mv <source> <dest>        move or rename a file\n"
		"get <local file>          import a file from the computer\n"
		"put <file>                export a file to the computer\n"
		"exit                      quit\n"
		"Paths: name, folder/name, root/name, /name\n";
}
