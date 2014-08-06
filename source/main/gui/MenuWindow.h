#ifndef __MENU_WINDOW_H_
#define __MENU_WINDOW_H_

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(MenuWindow, "newmenu.layout");

class MenuWindow :
	public wraps::BaseLayout,
	public RoRSingleton<MenuWindow>,
	public ZeroedMemoryAllocator
{
	friend class RoRSingleton<MenuWindow>;
	MenuWindow();
	~MenuWindow();

public:
	void Show();
	void Hide();

private:
	void eventMouseButtonClickSelectButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickSettingButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickAboutButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickExitButton(MyGUI::WidgetPtr _sender);

	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mSelTerrButton, "Select_Terrain");
	MyGUI::Button* mSelTerrButton;

	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mSettingsButton, "settings");
	MyGUI::Button* mSettingsButton;
	
	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mAboutButton, "about");
	MyGUI::Button* mAboutButton;
	
	ATTRIBUTE_FIELD_WIDGET_NAME(MenuWindow, mCloseButton, "exit");
	MyGUI::Button* mCloseButton;
};

#endif