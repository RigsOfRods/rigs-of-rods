#pragma once

#include "ForwardDeclarations.h"
#include "BaseLayout.h"

namespace RoR
{

namespace GUI
{
	
ATTRIBUTE_CLASS_LAYOUT(RigEditorMenubarLayout, "rig_editor_menubar.layout");
class RigEditorMenubarLayout : public wraps::BaseLayout
{

public:

	RigEditorMenubarLayout(MyGUI::Widget* _parent = nullptr);
	virtual ~RigEditorMenubarLayout();

protected:

	//%LE Widget_Declaration list start
	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorMenubarLayout, m_rig_editor_menubar, "rig_editor_menubar");
	MyGUI::MenuBar* m_rig_editor_menubar;

	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorMenubarLayout, m_item_file, "item_file");
	MyGUI::MenuItem* m_item_file;

	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorMenubarLayout, m_file_popup, "file_popup");
	MyGUI::PopupMenu* m_file_popup;

	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorMenubarLayout, m_file_popup_item_open, "file_popup_item_open");
	MyGUI::MenuItem* m_file_popup_item_open;

	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorMenubarLayout, m_file_popup_item_save, "file_popup_item_save");
	MyGUI::MenuItem* m_file_popup_item_save;

	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorMenubarLayout, m_file_popup_item_close, "file_popup_item_close");
	MyGUI::MenuItem* m_file_popup_item_close;

	//%LE Widget_Declaration list end
};

} // namespace GUI

} // namespace RoR

