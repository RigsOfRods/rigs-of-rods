// based on the MyGUI UnitTests

#pragma once

#include "MyGUI_OverlappedLayer.h"
#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"

namespace MyGUI
{

    class RTTLayer : public OverlappedLayer
    {
        MYGUI_RTTI_DERIVED(RTTLayer)

      public:
        RTTLayer();
        virtual ~RTTLayer();

        virtual void deserialization(xml::ElementPtr _node, Version _version);
        virtual void renderToTarget(IRenderTarget *_target, bool _update);

        void setTextureSize(const IntSize &_size);
        void setTextureName(const std::string &_name);

      private:
        MyGUI::ITexture *mTexture;
        IntSize          mTextureSize;
        std::string      mTextureName;
        bool             mOutOfDate;
    };

} // namespace MyGUI
