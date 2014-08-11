#ifndef __ABOUT_WINDOW_H_
#define __ABOUT_WINDOW_H_

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(AboutWindow, "about.layout");

class AboutWindow :
	public wraps::BaseLayout,
	public RoRSingleton<AboutWindow>,
	public ZeroedMemoryAllocator
{
	friend class RoRSingleton<AboutWindow>;
	AboutWindow();
	~AboutWindow();

public:
	void Show();
	void Hide();

private:
	void eventMouseButtonClickBackButton(MyGUI::WidgetPtr _sender);

	ATTRIBUTE_FIELD_WIDGET_NAME(AboutWindow, mBackButton, "backbtn");
	MyGUI::Button* mBackButton;

	ATTRIBUTE_FIELD_WIDGET_NAME(AboutWindow, mVersionWord, "ror_version");
	MyGUI::EditBox* mVersionWord;

	ATTRIBUTE_FIELD_WIDGET_NAME(AboutWindow, mNetVersionWord, "net_version");
	MyGUI::EditBox* mNetVersionWord;
};

#endif