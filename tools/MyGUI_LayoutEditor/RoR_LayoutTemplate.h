#pragma once

#include "ForwardDeclarations.h"
#include "BaseLayout.h"

namespace RoR
{

namespace GUI
{
	
ATTRIBUTE_CLASS_LAYOUT(#{Panel_Name}, "#{Layout_Name}");
class #{Panel_Name} : public wraps::BaseLayout
{

public:

	#{Panel_Name}(MyGUI::Widget* _parent = nullptr);
	virtual ~#{Panel_Name}();

protected:

	//%LE Widget_Declaration list start
	//%LE Widget_Declaration list end
};

} // namespace GUI

} // namespace RoR
