#pragma once

#include "ForwardDeclarations.h"
#include "BaseLayout.h"

namespace RoR
{

namespace GUI
{
	
ATTRIBUTE_CLASS_LAYOUT(RigEditorDeleteMenuLayout, "rig_editor_delete_menu.layout");
class RigEditorDeleteMenuLayout : public wraps::BaseLayout
{

public:

	RigEditorDeleteMenuLayout(MyGUI::Widget* _parent = nullptr);
	virtual ~RigEditorDeleteMenuLayout();

protected:

	//%LE Widget_Declaration list start
	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorDeleteMenuLayout, m_rig_editor_delete_menu, "rig_editor_delete_menu");
	MyGUI::Window* m_rig_editor_delete_menu;

	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorDeleteMenuLayout, m_delete_nodes_button, "delete_nodes_button");
	MyGUI::Button* m_delete_nodes_button;

	ATTRIBUTE_FIELD_WIDGET_NAME(RigEditorDeleteMenuLayout, m_delete_beams_button, "delete_beams_button");
	MyGUI::Button* m_delete_beams_button;

	//%LE Widget_Declaration list end
};

} // namespace GUI

} // namespace RoR

