#pragma once
#include "Controller.h"
#include <regex>
#include <string>

class UI
{
	const std::string COMMAND_PREFIX = "root> ";
	const std::regex ONE_ARGUMENT;
	const std::regex TWO_ARGUMENTS;
	Controller controller;

	void mkdir(const std::string&) noexcept(false);
	void create(const std::string&) noexcept(false);
	void echo(const std::string&, const std::string&) noexcept(false);
	void cat(const std::string&) noexcept(false);
	void cd(const std::string&) noexcept(false);
	void CD() noexcept(false);
	void stat(const std::string&) noexcept(false);
	void ls() noexcept(false);
	void rm(const std::string&) noexcept(false);
	void cp(const std::string&, const std::string&) noexcept(false);
	void mv(const std::string&, const std::string&) noexcept(false);
	void put(const std::string&) noexcept(false);
	void get(const std::string&) noexcept(false);
	void rename(const std::string&, const std::string&) noexcept(false);

public:

	UI();

	void run();
};