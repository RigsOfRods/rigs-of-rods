// based on the MyGUI UnitTests

#pragma once

#include "MyGUI_Prerequest.h"
#include "MyGUI_Types.h"
#include "MyGUI_OverlappedLayer.h"

namespace RoR {

/// RTT texture 'slot' within MyGUI system; we can only create them, not destroy, so we reuse.
class RTTLayer : public MyGUI::OverlappedLayer
{
    MYGUI_RTTI_DERIVED( RTTLayer )

public:
    RTTLayer();
    virtual ~RTTLayer();

    void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version) override;
    void renderToTarget(MyGUI::IRenderTarget* _target, bool _update) override;

    void setTextureSize(const MyGUI::IntSize& _size); //!< Call before `createRttTexture()`
    void setTextureName(const std::string& _name); //!< Call before `createRttTexture()`

    const std::string& getTextureName() const { return mTextureName; }

    void createRttTexture();
    void destroyRttTexture();

private:
    MyGUI::ITexture* mTexture;
    MyGUI::IntSize mTextureSize;
    std::string mTextureName;
    bool mOutOfDate;
};

/// Global object, see `RoR::GUIManager::GetRttLayerManager()`
class RTTLayerManager
{
public:
    RTTLayer* CreateOrReuseRttLayer();
    void RecycleRttLayer(RTTLayer* obj);
private:
    int mNextRenderdashRttNum = 1;
    std::vector<RTTLayer*> mReusableRttLayers; //!< MyGUI allows dynamic allocation of RTT layers, but not destruction - we reuse.
};

} // namespace RoR
