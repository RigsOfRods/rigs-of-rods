#include "App.h"
#include "Util.h"

using namespace ckeys;


//  Graphics draw
///-----------------------------------------------------------------------------
void App::Graph()
{


	int yF = set.iFontH;

	//  Fps
	if (set.bFps)
	{
		text_character_size = (yF);
		Clr(155,215,255);
		str = f2s(fps,1,3);  // 1/dt
		Txt(set.xwSize - 70, 2);
	}

	//  Keys info
	if (set.bInfo)
	{
		text_character_size = (yF-3);
		Clr(115,125,155);
		int x = set.xwSize - 90;

		str = "Layers: " + i2s(keys.Lnum);
		Txt(x, set.ywSize - yF);
		str = "Keys: " + i2s(keys.keys.size());
		Txt(x, set.ywSize - yF*2);
	}


	//  draw keyboard
	//----------------------------------------------------------------
	const float sc = set.fScale;
	const int yL = set.iFontH * sc;
	const int xl = !set.bList ? 20 :  // left margin x
					set.bListSimple ? 110 : 230;
	xMax = 0;  yMax = 0;
	const rgb* c;

	//  key under mouse, for info
	Key* kum = nullptr;
	if (set.bBold)
		bold = true;

	if (set.bLayout)
	for (Key& k : keys.keys)
	{
		int x = k.x * sc + xl, y = k.y * sc + 20,
			x2 = x + k.w * sc, y2 = y + k.h * sc;

		//  key vars
		int q = 0, r = 1;  //  frame offset and thickness

		KClr kc = k.on ? KC_Pressed : k.clr;
		if (set.bKLL && !k.inKll ||
			set.bVK && !k.inVK ||
			set.bScan && k.scan == -1)
			kc = KC_Missing;

		#define isLay(l)  (set.bL[l] && !k.strL[l].isEmpty())
		int l = k.layer;

		//  draw  []
		c = set.getC(kc,l,0);  Rect( x,y, x2,y2,    c->r,c->g,c->b);
		c = set.getC(kc,l,1);  Frame(x,y, x2,y2, r, c->r,c->g,c->b);
		c = set.getC(kc,l,2);  Clr(c->r,c->g,c->b);
		bool Lany = false;
		if (!k.on)
		{
			for (int i=0; i <= keys.Lnum; ++i)
			if (isLay(i))
			{
				c = set.getC(KC_Lnum, i, 1);  Frame(x+q, y+q, x2-q, y2-q, r, c->r,c->g,c->b);
				++q;  Lany = true;
			}
		}

		//  mouse over
		if (xm >= x && xm <= x2 && ym >= y && ym <= y2)
		{	kum = &k;
			q = -1;  r = 1;
			c = &set.clrOver;  Frame(x+q, y+q, x2-q, y2-q, r, c->r,c->g,c->b);
		}

		//  caption  ----
		str = set.bKLL ? k.sKll : k.Caption();
		bool ln2 = str.find("\n") != std::string::InvalidPos;

		bool Loff = set.bL[0] || !Lany;  // force or empty
		text_character_size = (k.sc * sc);
		if (Loff)  Txt(x + 4, y + 4);

		//  layer label(s)  ----
		int xl = x + 4 + (Loff && ln2 ? 18.f*sc : 0);
		int yl = y + 4 + (Loff ? yL : 0);

		if (!k.on)
		for (int i=0; i <= keys.Lnum; ++i)
		if (isLay(i))
		{
			str = k.strL[i];
			c = set.getC(KC_Lnum, i, 2);
			Clr(c->r,c->g,c->b);
			Txt(xl, yl);  yl += yL;
		}
		#undef isLay

		//  get max size
		if (x2 > xMax)  xMax = x2;
		if (y2 > yMax)  yMax = y2;
	}


	//  key info
	//--------------------------------
	int x,y, x1,y1;
	if (!help && !options && !graphics)
	{
		//bold = true;
		text_character_size = (yF-1);  int yL = yF + 4;
		x = set.xGuiSize + 30;  x1 = x + 170;
		y = set.ywSize - set.yGuiSize + 20;  y1 = y;

		if (!kum)
		{	str = "Key info";// +i2s(xm)+ " "+i2s(ym);
			Clr(150,180,220);  Txt(x, y);  y += 3*yL;
		}else
		{	//  1st col  ----
			str = "Key:  " + kum->Caption();  str.replace("\n","  ");
			Clr(190,220,250);  Txt(x, y);  y += yL;

			for (int i=0; i <= keys.Lnum; ++i)
			if (!kum->strL[i].isEmpty())
			{
				str = "L" + i2s(i) + ":  " + kum->strL[i];
				c = set.getC(KC_Lnum, i, 2);
				Clr(c->r,c->g,c->b);
				Txt(x+9, y);  y += yL;
				if (y+yL > set.ywSize) {  y = y1;  x += 350;  }  //out
			}

			//  2nd col  ----
			str = "Kll: " + kum->sKll;
			Clr(100,170,250);  Txt(x1+15, y1);  y1 += yL;
			str = "Json: " + kum->sJson;
			Clr(165,165,240);  Txt(x1, y1);  y1 += yL;
			str = "Scan: " + keys.kll2scan[kum->sKll];
			Clr(140,160,200);  Txt(x1-6, y1);  y1 += yL;
			str = "  VK: ";  str += kum->sVK;  //km->inVK ? "1" : "0";
			Clr(140,140,180);  Txt(x1+4, y1);  y1 += yL;
		}
	}


	//  keys pressed list
	//--------------------------------
	if (!set.bList)  return;

	#ifdef _WIN32
	text_character_size = (yF - 3);
	x = 10;  x1 = x + 110;  y = 25;
	char s[200];

	//  header
	Clr(110,140,170);
	if (set.bListSimple)
	{	str = "Pressed";  x1 = x;  }
	else
		str = "VK  SC  ext  Name";
	Txt(x,y);

	text_character_size = (yF);
	y += yF + 8;
	Clr(140,210,255);

	//  list
	for (auto& kc : keys.keyCodes)
	{
		if (!set.bListSimple)
		{
			//  full info  vk, sc, ext
			sprintf(s, "%02X  %02X", kc.vk, kc.sc);
			str = s;
			Clr(140,210,255);  bold = false;
			Txt(x,y);

			str = i2s(kc.ext);
			Clr(140,150,155);
			Txt(x1 - 25, y);
		}

		//  key name
		std::string sk = keys.vk2str[kc.vk];

		keys.ReplacePressed(sk);  //**

		str = sk;
		Clr(200,230,255);  bold = true;
		Txt(x1, y);
		y += yF + 2;
	}
	bold = false;
	#endif
}
