#include "Keys.h"
#include "KeyNames.h"
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include "../libs/jsmn.h"
#include "Util.h"
using namespace std;


//  read layout from json file
//-----------------------------------------------------------------------------------------------------------
bool Keys::LoadJson(string path, bool logOut)
{
	//  open
	ifstream f;
	f.open(path);
	if (!f.is_open())
		return false;

	ofstream of;
	if (logOut)
	{
		of.open(path + ".log");
		if (!of.good())
			logOut = false;
	}

	stringstream ss;  ss << f.rdbuf();
	string str = ss.str();
	const char* s = str.c_str();

	//  jsmn json vars
	int i,r;
	jsmn_parser p;
	jsmntok_t t[512];  // const max
	jsmn_init(&p);

	//  parse
	string q;  //, re;
	r = jsmn_parse(&p, s, strlen(s), t, sizeof(t)/sizeof(t[0]));
	if (r < 0)
		return false;
	//	re += "Failed to parse JSON: " + r;

	//  layout
	int x=0, y=0;
	float w=1.f, h=1.f;  string prim;

	//  params
	const float sc = 1.3f;
	const int  sx = 40 * sc, sy = sx, se = 5,  // size x,y, empty margin
			 yfnt = 14 * sc, x0 = 0, y0 = 0;  // font, start pos

	//  iterate all
	//-----------------------------------------------------------
	for (i = 1; i < r; i++)
	{
		if (t[i].type == JSMN_ARRAY)
		{   x = 0;  y += sy;  w = 1.f;  h = 1.f;  // next row
		}else
		if (t[i].type == JSMN_STRING)
		{
			//  text
			string s = str.substr(t[i].start, t[i].end - t[i].start);
			bool len2 = s.length() > 2;

			if (!len2 && s[0]>='a' && s[0]<='z')
				prim = s;  // primitive

			else  //  custom caption
			if (len2 && s[0]=='n' && s[1]==':')
			{
				Key& k = keys.back();  // prev key
				s = s.substr(2);

				sf::String ws = s;
				ReplacePlayer(s,ws);

				k.name = ws;
				of << "Name: " << s << "\n";
			}
			else  //  scan code
			if (len2 && s[0]=='s' && s[1]==':')
			{
				Key& k = keys.back();
				s = s.substr(2);

				uint16_t x;  // hex str to byte
				stringstream ss;
				ss << hex << s;  ss >> x;

				k.scan = x;
				of << "Scan: " << s << " s " << k.sVK << "\n";

				kll2scan[k.sKll] = s;  //`
			}
			else  //  add key  --------
			{
				string js = s, sVK, sk;
				bool ext, has2;
				sf::String ws = ReplaceJson(s, sVK, sk, ext, has2);  //**

				//  setup
				Key k;
				k.x = x0 + x;  k.y = y0 + y;
				k.w = sx * w - se;  k.h = sy * h - se;
				k.str = ws;  k.sJson = js;
				k.SetClr();

				//  font scale
				float sf = w < 0.7f ? 0.6f/*ck4 mini*/ : 0.8f;
				k.sc = sf * yfnt;

				x += w * sx;  // add x
				w = 1.f;  h = 1.f;  // reset dim


				//  vk to key  ------
				int vk = str2vk[sVK];
				bool ok = vk > 0;
				char hh[4];
				sprintf(hh, "%02X", vk);
				k.sVK = !ok ? "" : hh;
				k.inVK = ok;

				if (logOut)
					of << hh << "  " << sVK << (vk==0 ? "\t\t!!!" : "") << endl;

				if (ok)  // if found
				{
					int kk = keys.size()+1;
					if (ext)  kk += vk_EXTRA;  // numpad
					vk2key[vk] = kk;
				}

				str2key[sk] = keys.size()+1;
				k.sKll = sk;  // for info


				keys.push_back(k);  // add key
		}   }
		else
		if (t[i].type == JSMN_PRIMITIVE)
		{
			string p = str.substr(t[i].start, t[i].end - t[i].start);
			if (p[0]>='a' && p[0]<='z')
				prim = p;
			else
			{   float f = 0.f;
				sscanf(p.c_str(), "%f", &f);

				if (prim == "x")  x += f * sx;
				else
				if (prim == "y")  y += f * sy;
				else
				if (prim == "w")  w = f;
				else
				if (prim == "h")  h = f;
			}
		}
	}
	return true;
}
