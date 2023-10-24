#define _CRT_SECURE_NO_DEPRECATE // Enabling fread and fwrite functions to compile
#include "UI.h"

UI::UI() :ONE_ARGUMENT(".*?\\s(.*)"), TWO_ARGUMENTS(".*?\\s(.*?)\\s(.*)") {}

void UI::run()
{
	std::string input;

	while (true)
	{
		std::cout << COMMAND_PREFIX;
		std::getline(std::cin, input);

		try
		{
			if (input == "exit") {
				break;
			}
			else if (input.substr(0, 5) == "mkdir") {
				// To do: checking the command syntax
				// regex
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				mkdir(path);
			}
			else if (input.substr(0, 6) == "create") {
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				create(path);
			}
			else if (input.substr(0, 4) == "echo") {
				std::smatch match;
				std::regex_match(input, match, TWO_ARGUMENTS);
				std::string path = match[1].str();
				std::string data = match[2].str();
				echo(path, data);
			}
			else if (input.substr(0, 2) == "cp") {
				std::smatch match;
				std::regex_match(input, match, TWO_ARGUMENTS);
				std::string str1 = match[1].str();
				std::string str2 = match[2].str();
				cp(str1, str2);
			}
			else if (input.substr(0, 2) == "mv") {
				std::smatch match;
				std::regex_match(input, match, TWO_ARGUMENTS);
				std::string str1 = match[1].str();
				std::string str2 = match[2].str();
				mv(str1, str2);
			}
			else if (input.substr(0, 6) == "rename")
			{
				std::smatch match;
				std::regex_match(input, match, TWO_ARGUMENTS);
				std::string name = match[1].str();
				std::string str = match[2].str();
				rename(name, str);
			}
			else if (input.substr(0, 3) == "cat") {
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				cat(path);
			}
			else if (input.substr(0, 2) == "cd")
			{
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				cd(path);
			}
			else if (input.substr(0, 2) == "CD")
			{
				CD();
			}
			else if (input.substr(0, 4) == "stat")
			{
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				stat(path);
			}
			else if (input.substr(0, 2) == "ls")
			{
				ls();
			}
			else if (input.substr(0, 2) == "rm")
			{
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				rm(path);
			}
			else if (input.substr(0,3) == "put")
			{
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				put(path);
			}
			else if (input.substr(0, 3) == "get")
			{
				std::smatch match;
				std::regex_match(input, match, ONE_ARGUMENT);
				std::string path = match[1].str();
				get(path);
			}
		}
		catch (std::exception e)
		{
			std::cout << e.what();
		}
	}
}


void UI::mkdir(const std::string& str) noexcept(false)
{
	controller.mkdir(str);
}

void UI::create(const std::string& str) noexcept(false)
{
	controller.create(str);
}

void UI::echo(const std::string& name, const std::string& str)
{
	controller.echo(name, str);
}

void UI::cat(const std::string& str)
{
	controller.cat(str);
}

void UI::rm(const std::string& str) noexcept(false)
{
	controller.rm(str);
}

void UI::cd(const std::string& str) noexcept(false)
{
	controller.cd(str);
}

void UI::CD() noexcept(false)
{
	controller.CD();
}

void UI::stat(const std::string& str) noexcept(false)
{
	controller.stat(str);
}

void UI::ls() noexcept(false)
{
	controller.ls();
}

void UI::cp(const std::string& str1, const std::string& str2) noexcept(false)
{
	controller.cp(str1, str2);
}

void UI::mv(const std::string& str1, const std::string& str2) noexcept(false)
{
	controller.mv(str1, str2);
}

void UI::put(const std::string& str) noexcept(false)
{
	controller.put(str);
}

void UI::get(const std::string& str) noexcept(false)
{
	controller.get(str);
}

void UI::rename(const std::string& name, const std::string& str) noexcept(false)
{
	controller.rename(name, str);
}