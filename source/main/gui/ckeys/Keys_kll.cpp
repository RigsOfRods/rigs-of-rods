#include "Keys.h"
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include "Util.h"
using namespace std;  using namespace ckeys;


// kll syntax:
//
//   pre"sname": pre2"name";  #comment\n
//   S0x sscan  \mid       \end
//
// examples:
//   U"A": U"B";  U"1": s(1);  U";": '(';
//   S0x39: U"F1";  S0x78: CONS"STOP";  S0x4E: U"P+";
//   U"V":SYS"SLEEP";
//	 U"Delete": mouseOut(3, 0, 0);
//   S0x6D: U"Alt"+U"Esc";  S0x77:+ shift();


//  kll parser state variables
struct SParser
{
	bool comment, apos, scan, skip;  // flags
	int name0, apos0, scan0, mid0;  // start pos
	string name, sname, sscan, pre;

	SParser()
	{
		Reset();
	}
	void Reset()
	{
		comment = apos = scan = skip = false;
		name0 = apos0 = scan0 = mid0 = 0;
		name = sname = sscan = pre = "";
	}
	void MidReset()
	{
		comment = apos = false;
		name0 = apos0 = scan0 = 0;
		name = "";
	}
};


//  load layer, from kll file
//------------------------------------------------------------------------------------------------
bool Keys::LoadKll(string path, int layer, bool logOut)
{
	//  open
	ifstream f;
	f.open(path);
	if (!f.is_open())
		return false;

	ofstream of;
	if (logOut)
	{
		of.open(path+".log", ofstream::out);
		if (!of.good())
			logOut = false;
	}

	//  read whole file as string s
	string s;
	#define LN 512
	char ln[LN];
	int l=0;
	while (!f.eof())
	{
		f.getline(ln,LN-1);  ++l;
		//  skip first 5 lines
		if (l > 5)
		{
			s += string(ln);
			s += "\n";
		}
	}

	//  parse kll
	//------------------------------------------------
	int pos=0, len = s.length();
	SParser p;

	//  foreach char in string s
	while (pos++ < len)
	{
		char ch = s[pos];
		//of << ch << "\n";

		//  comment, skip until \n
		if (ch=='#')
		{
			p.comment = true;
		}
		else  //  end of line, reset
		if (ch=='\n')
		{
			p.Reset();
		}
		//  normal  -----------------
		else if (!p.comment)
		{
			//  scan code  S0x..
			if (pos>=2 && ch=='x' && s[pos-1]=='0' && s[pos-2]=='S')
			{
				p.scan0 = pos+1;  p.scan = 1;
			}
			//  outside apos
			else if (p.apos == 0)
			{
				if (ch==':')  // middle of mapping
				{
					//  skip :+
					if (pos+1 < len && s[pos+1]=='+')
						p.skip = 1;
					else
					{
						if (p.scan == 1)
							p.sscan = s.substr(p.scan0, pos - p.scan0);
						else
							p.sname = p.name;

						p.mid0 = pos+1;
						//  trim front spaces
						while (s[p.mid0]==' ' && p.mid0 < len)  ++p.mid0;
						p.MidReset();
					}
				}
				else if (ch==';' && !p.skip)  // end of mapping
				{
					int id = -1;
					if (p.scan)
					{
						id = str2key[p.name]-1;
						if (layer == 0)
							kll2scan[p.name] = p.sscan;

						if (logOut)
							of << "sc: " << p.sscan << "  n: " << p.name <<
								  "  id: " << id << (id==-1 ? "\t\t!!!" : "") << "\n";
					}else{
						if (p.name.empty())
							p.name = s.substr(p.mid0, pos - p.mid0);
						id = str2key[p.sname]-1;

						if (logOut)
							of << "sn: " << p.sname << "  n: " << p.name <<
								  "  id: " << id << (id==-1 ? "\t\t!!!" : "") << "\n";
					}
					if (id >= 0)
					{
						Key& k = keys[id];
						k.inKll = true;

						sf::String ws = ReplaceKll(p.name);  //**

						if (layer > 0)
							k.strL[layer] = ws;
					}
					p.Reset();
				}
				else if (ch=='U')
				{
					p.name0 = pos;  p.pre = "U";
				}
			}
			//  apos "name"
			if (ch=='"')
			{
				if (p.apos == 0)  // 1st, start"
				{
					p.apos0 = pos+1;
					p.apos = 1;
				}
				else  // 2nd, end"
				{
					if (!p.name.empty())
						p.sname = p.name;

					if (!p.name.empty())
						p.name += "+";  // add, s[pos+1]=='+';
					p.name += s.substr(p.apos0, pos - p.apos0);

					//of << "name: " << name << "\n";
					p.apos = 0;  //name0 = pos;
				}
			}
		}
	}
	return true;
}
