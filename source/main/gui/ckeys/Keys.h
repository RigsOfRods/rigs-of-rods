#pragma once
#include <string>
#include <vector>
#include <map>
#include <list>

#include "Settings.h"  // Lmax

namespace ckeys {


//  key for pressed list
struct KeyCode
{
	int vk=0, sc=0, ext=0;

	bool operator==(const KeyCode& o) const
	{	return vk == o.vk && sc == o.sc && ext == o.ext;  }

	bool operator<(const KeyCode& o) const
	{	return vk < o.vk || sc < o.sc || ext < o.ext;  }
};


//  single key, for Gui layout
struct Key
{
	//  from json  ----
	int x,y, w,h;     // position, dimensions
	float sc = 1.f;   // scale

	std::string str;     // key name and caption, shown on Gui

	//  optional json
	std::string name;    // custom caption on Gui, overrides str

	std::string Caption()
	{	return name.empty() ? str : name;  }

	int16_t scan = -1;  // custom scan code, -1 none

	std::string sJson;  // test original name in json

	//  color set, based on name
	KClr clr = KC_Normal;
	int layer = 0;
	void SetClr();

	//  from kll  ----
	std::string sKll;     // test name for kll
	bool inKll = false;  // test kll

	//  caption on other layers 2,3,..
	std::string strL[Settings::Lmax];

	//  vk test  ----
	bool inVK = false;
	std::string sVK;

	//  var, from key hook
	bool on = false;    // pressed
};


//  Keyboard layout
class Keys
{
public:
	Keys();
	Settings* set = nullptr;

	void LoadIndex();

	bool LoadJson(std::string path, bool logOut = true);
	bool LoadKll(std::string path, int layer, bool logOut = true);
	void Destroy();

	//  replace key names, format
	std::string ReplaceJson(std::string& s, std::string& sVK, std::string& sk, bool& ext, bool& has2);
	std::string ReplaceKll(const std::string& name);
	void ReplaceArrows(const std::string& s, std::string& ws);
	void ReplacePlayer(const std::string& s, std::string& ws);  // util
	void ReplacePressed(std::string& sk);


	bool Init(Settings* pSet);
	std::vector<std::string> files;  // .json layouts
	std::vector<char> vCmb;  // for combo, all files zero separated

	//  filled in ctor, from csKeyNames
	std::map<int, std::string> vk2str;
	std::map<std::string, int> str2vk;


	//  keyboard layout  ----
	int Lnum = 0;  // layers count, last number
	std::vector<Key> keys;

	//  vk code to key id,  for hook
	//  maps have +1,  0 if not found
	std::map<int, int> vk2key;


	//  str caption (1 row) to keys id, for kll layers
	std::map<std::string, int> str2key;
	//  from kll defaultMap
	std::map<std::string, std::string> kll2scan;


	//  keys pressed list
	std::list<KeyCode> keyCodes;
};

} // namespace ckeys
