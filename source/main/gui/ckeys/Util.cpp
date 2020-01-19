#include "Util.h"
#include <sstream>
#include <iomanip>
#include <regex>
#include <fstream>
#include <dirent.h>
using namespace std;


//  format
string i2s(const int v, const char width)
{
	ostringstream s;
	if (width != 0)  s.width(width);  //s.fill(fill);
	s << fixed << v;
	return s.str();
}

string f2s(const float v, const char precision, const char width)
{
	ostringstream s;
	if (width != 0)  s.width(width);
	s << fixed << setprecision(precision) << v;
	return s.str();
}

//  string
vector<string> split(const string& s, const string& reg)
{
	regex re(reg);
	sregex_token_iterator
		first{s.begin(), s.end(), re, -1},  // -1 split
		last;
	return {first, last};
}

string strlower(const string& s)
{
	string ss = s;
	transform(ss.begin(), ss.end(), ss.begin(), ::tolower);
	return ss;
}


//  replace
bool replK(string& str, const string& what, const string& to)
{
	size_t p = str.find(what);
	bool rep = p != string::npos;
	if (rep)
		str.replace(p, what.length(), to);
	return rep;
}

bool replK(wstring& str, const wstring& what, const wstring& to)
{
	size_t p = str.find(what);
	bool rep = p != wstring::npos;
	if (rep)
		str.replace(p, what.length(), to);
	return rep;
}


//  ends with
bool endsWith(string const &str, string const &ending)
{
	if (str.length() < ending.length())
		return false;
	return str.compare(str.length() - ending.length(), ending.length(), ending) == 0;
}


//  file
bool exists(const string& name)
{
	ifstream f(name.c_str());
	return f.good();
}

//  dir
int getDir(string dir, vector<string> &files)
{
	DIR *dp = dp = opendir(dir.c_str());
	struct dirent *dirp;
	if(dp == NULL)
	{
		//cout << "Error(" << errno << ") opening " << dir << endl;
		return errno;
	}

	while ((dirp = readdir(dp)) != NULL)
	{
		files.push_back(string(dirp->d_name));
	}
	closedir(dp);
	return 0;
}
