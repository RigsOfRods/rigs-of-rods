#include "AppDraw.h"


//  draw utils
//------------------------------------------------------------------

//  write out text
int AppDraw::Txt(int x, int y, bool draw)
{
	text.setString(str);
	text.setStyle(bold ? sf::Text::Bold : sf::Text::Regular);
	text.setColor(clr);
	text.setPosition(x, y);
	if (draw)  pWindow->draw(text);
	return text.getLocalBounds().width;  // advance x pos
}

//  clear rect
void AppDraw::Rect(int x, int y,  int sx, int sy,
		  sf::Uint8 r, sf::Uint8 g, sf::Uint8 b)
{
	pBackgr->setScale(sx-x, sy-y);
	pBackgr->setPosition(x, y);
	pBackgr->setColor(sf::Color(r, g, b));
	pWindow->draw(*pBackgr);
}

//  frame rect, inefficient
void AppDraw::Frame(int x, int y,  int sx, int sy,  int d,
		sf::Uint8 r, sf::Uint8 g, sf::Uint8 b)
{
	Rect(x,   y,    sx-d, y+d,  r, g, b);  // top
	Rect(x,   sy-d, sx-d, sy,   r, g, b);  // bottom
	Rect(x,   y,    x+d,  sy-d, r, g, b);  // left
	Rect(sx-d,y,    sx,   sy,   r, g, b);  // right
}
