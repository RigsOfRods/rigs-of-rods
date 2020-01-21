#include "Application.h"
#include "ContentManager.h"
#include "Keys.h"
#include "KeyNames.h"
#include <string>
#include <fstream>
#include <sstream>
#include <string.h>
#include "Util.h"

#include <Ogre.h>

using namespace ckeys;

#ifdef GetObject // WinAPI :(
#  undef GetObject
#endif

//  read layout from json file
//  Format: export from http://www.keyboard-layout-editor.com
//    > root is [] of button rows
//      > button row is [] with sequential data
//        > "" means 'add button' and contains button ID - the ID may appear more than once to create complex buttons, such as Enter
//        > {} means 'update params' (x/y -> advance next button position, w/h-> set next button size)
//-----------------------------------------------------------------------------------------------------------
bool Keys::LoadJson(std::string path, bool logOut)
{
    // Load keyboard layout file
    rapidjson::Document j_doc;
    if (!RoR::App::GetContentManager()->LoadAndParseJson(path, Ogre::RGN_AUTODETECT, j_doc))
    {
        return false; // Error already logged
    }

    // Validate the JSON
    if (!j_doc.IsArray())
    {
        RoR::LogFormat("[RoR|VirtualKeyboard] Invalid keyboard layout file '%s' - root element must be an array", path.c_str());
        return false;
    }

	//  layout
	int x=0, y=0;
	float w=1.f, h=1.f;  std::string prim;

	//  params
	const float sc = 1.3f;
	const int  sx = 40 * sc, sy = sx, se = 5,  // size x,y, empty margin
			 yfnt = 14 * sc, x0 = 0, y0 = 0;  // font, start pos

    for (rapidjson::Value& j_row: j_doc.GetArray())
    {
        x = 0;  y += sy;  w = 1.f;  h = 1.f;  // next row

        for (rapidjson::Value& j_item: j_row.GetArray())
        {
            if (j_item.IsString())
            {
                std::string s = j_item.GetString();
                bool len2 = s.length() > 2;

                if (len2 && s[0]=='n' && s[1]==':') //  custom caption
                {
                    Key& k = keys.back();  // prev key
                    s = s.substr(2);

                    std::string ws = s;
                    ReplacePlayer(s,ws);

                    k.name = ws;
                }
                else  if (len2 && s[0]=='s' && s[1]==':') //  scan code
                {
                    Key& k = keys.back();
                    s = s.substr(2);

                    uint16_t x;  // hex str to byte
                    std::stringstream ss;
                    ss << std::hex << s;  ss >> x;

                    k.scan = x;

                    kll2scan[k.sKll] = s;  //`
                }
                else  //  add key  --------
                {
                    std::string sVK, sk;
                    bool ext, has2;
                    std::string ws = ReplaceJson(s, sVK, sk, ext, has2);  //**

                    //  setup
                    Key k;
                    k.x = x0 + x;  k.y = y0 + y;
                    k.w = sx * w - se;  k.h = sy * h - se;
                    k.str = ws;
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

                    if (ok)  // if found
                    {
                        int kk = keys.size()+1;
                        if (ext)  kk += 1000;  // numpad (orig: `#define	vk_EXTRA 1000`)
                        vk2key[vk] = kk;
                    }

                    str2key[sk] = keys.size()+1;
                    k.sKll = sk;  // for info


                    keys.push_back(k);  // add key
                }
            }
            else if (j_item.IsObject()) // Update params
            {
                if (j_item["x"].IsString()) { x += j_item["x"].GetFloat() * sx; }
                if (j_item["y"].IsString()) { y += j_item["y"].GetFloat() * sy; }
                if (j_item["w"].IsString()) { w = j_item["w"].GetFloat(); }
                if (j_item["h"].IsString()) { h = j_item["h"].GetFloat(); }
            }
        }
    }

	return true;
}
