#include "Keys.h"
#include "KeyNames.h"
#include "Settings.h"
#include "Util.h"
using namespace std;  using namespace ckeys;


//  ctor
Keys::Keys()
{
	//  filled vk to str maps from csKeyNames
	for (int k=0; k < 256; ++k)
	{
		string s = csKeyNames[k];
		vk2str[k] = s;
		str2vk[s] = k;
	}
}

//  find data path
//-----------------------------------------------------------
bool Keys::Init(Settings* pSet)
{
	set = pSet;
	files.clear();
	vCmb.clear();

	//  get .json files from data dir, into combo
	vector<string> vf;
	getDir(set->data, vf);

	for (auto s : vf)
	{
		if (endsWith(s, ".json"))
		{
			string ss = s;  replK(ss, ".json", "");
			files.push_back(ss);
		}
	}
	for (auto f : files)
	{
		for (auto c : f)
			vCmb.push_back(c);

		vCmb.push_back(0);
	}
	vCmb.push_back(0);
	return true;
}

void Keys::Destroy()
{
	keys.clear();
	vk2key.clear();
	str2key.clear();
	kll2scan.clear();
	Lnum = 0;
}

//  read layouts from file
void Keys::LoadIndex()
{
	int id = set->iCombo;
	bool log = set->logOut;

	if (id >= files.size())
		return;

	//  clear last
	Destroy();

	//  load klls
	string fname = files[id];
	string p = set->data;
	//	default map:  scan code to kll name
	LoadKll(p + "defaultMap" + fname + ".kll", 0, log);

	//	layer maps
	//  kll name to layer key/function
	vector<string> vf;
	getDir(p, vf);  // get any 1..9

	for (string s : vf)
	{
		if (endsWith(s, ".kll") && found(s, "layer") && found(s, fname))
		{
			//  get layer number, digit before .
			size_t pos = s.find('.');
			char l = s.substr(pos-1,1)[0] - '0';

			if (l >= 1 && l < Settings::Lmax)
			if (LoadKll(p + s, l, log))
				if (l > Lnum)  Lnum = l;
		}
	}
	//LoadKll(p + fname + "overlay.kll", 0);  // Fn to layer num
	//U"Function2" : layerShift(2);  # hold
	//U"Function3" : layerShift(3);  # hold
}
