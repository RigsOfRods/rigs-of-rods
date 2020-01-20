#pragma once
#include <vector>
#include <string>
#include <algorithm>

namespace ckeys {


//  format int, float to string
std::string i2s(const int v, const char width=0/*, const char fill=' '*/);
std::string f2s(const float v, const char precision=2, const char width=4);


//  split string
std::vector<std::string> split(
	const std::string& s, const std::string& reg);

//  string to lower
std::string strlower(const std::string& s);

//  found, substring ss in s
static bool found(const std::string& s, const std::string& ss)
{	return s.find(ss) != std::string::npos;  }


//  replace in key name
bool replK(std::string& str, const std::string& what, const std::string& to);
bool replK(std::wstring& str, const std::wstring& what, const std::wstring& to);

bool endsWith(std::string const &str, std::string const &ending);

//  file exists
bool exists(const std::string& name);

//  list directory contents
int getDir(std::string dir, std::vector<std::string> &files);

void strFindAndReplace(std::string& str, std::string const& needle, std::string const& patch)
{
	size_t off;
	while ((off = str.find(needle)) != std::string::npos)
	{
		str.replace(str.begin() + off, str.begin() + off + needle.length(), patch);
	}
}

} // namespace ckeys
