#ifndef __SETTINGS_WINDOW_H_
#define __SETTINGS_WINDOW_H_

#include "RoRPrerequisites.h"
#include "Singleton.h"
#include "mygui/BaseLayout.h"

ATTRIBUTE_CLASS_LAYOUT(SettingsWindow, "settings.layout");

class SettingsWindow :
	public wraps::BaseLayout,
	public RoRSingleton<SettingsWindow>,
	public ZeroedMemoryAllocator
{
	friend class RoRSingleton<SettingsWindow>;
	SettingsWindow();
	~SettingsWindow();

public:
	void Show();
	void Hide();

private:
	
	void eventMouseButtonClickBackButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickSaveButton(MyGUI::WidgetPtr _sender);

	/*void eventMouseButtonClickAboutButton(MyGUI::WidgetPtr _sender);
	void eventMouseButtonClickExitButton(MyGUI::WidgetPtr _sender);

	ATTRIBUTE_FIELD_WIDGET_NAME(SettingsWindow, mSelTerrButton, "Select_Terrain");
	MyGUI::Button* mSelTerrButton;

	ATTRIBUTE_FIELD_WIDGET_NAME(SettingsWindow, mSettingsButton, "settings");
	MyGUI::Button* mSettingsButton;
	*/

	ATTRIBUTE_FIELD_WIDGET_NAME(SettingsWindow, mBackButton, "backbtn");
	MyGUI::Button* mBackButton;
	
	ATTRIBUTE_FIELD_WIDGET_NAME(SettingsWindow, mSaveButton, "savebtn");
	MyGUI::Button* mSaveButton;
};

#endif