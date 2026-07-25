// based on the MyGUI UnitTests

#include "MyGUI_LayerItem.h"
#include "RTTLayer.h"
#include "MyGUI_Enumerator.h"
#include "MyGUI_FactoryManager.h"
#include "MyGUI_RenderManager.h"
#include "MyGUI_Gui.h"
#include "MyGUI_LayerNode.h"

using namespace RoR;

using namespace MyGUI;

RTTLayer::RTTLayer() :
    mTexture(nullptr),
    mOutOfDate(false)
{
}

RTTLayer::~RTTLayer()
{
    this->destroyRttTexture();
}

void RTTLayer::deserialization(xml::ElementPtr _node, Version _version)
{
    Base::deserialization(_node, _version);

    MyGUI::xml::ElementEnumerator propert = _node->getElementEnumerator();
    while (propert.next("Property"))
    {
        const std::string& key = std::string(propert->findAttribute("key"));
        const std::string& value = std::string(propert->findAttribute("value"));
        if (key == "TextureSize")
            setTextureSize(utility::parseValue<IntSize>(value));
        if (key == "TextureName")
            setTextureName(value);
    }

    this->createRttTexture();
}

void RTTLayer::renderToTarget(IRenderTarget* _target, bool _update)
{
    if (!mTexture)
        return;

    bool outOfDate = mOutOfDate || isOutOfDate();

    if (outOfDate || _update)
    {
        MyGUI::IRenderTarget* target = mTexture->getRenderTarget();
        if (target != nullptr)
        {
            target->begin();

            for (VectorILayerNode::iterator iter = mChildItems.begin(); iter != mChildItems.end(); ++iter)
                (*iter)->renderToTarget(target, _update);

            target->end();
        }
    }

    mOutOfDate = false;
}

void RTTLayer::setTextureSize(const IntSize& _size)
{
    if (mTextureSize == _size)
        return;

    mTextureSize = _size;

    ROR_ASSERT(mTextureSize.width && mTextureSize.height && "RTTLayer texture size must have non-zero width and height");

    mOutOfDate = true;
}

void RTTLayer::setTextureName(const std::string& _name)
{
    mTextureName = _name;

    mOutOfDate = true;
}

void RTTLayer::destroyRttTexture()
{
    if (mTexture)
    {
        mTexture->destroy();
        MyGUI::RenderManager::getInstance().destroyTexture(mTexture);
        mTexture = nullptr;
        LOG(fmt::format("[RoR|Dashboard] Destroyed texture '{}' in RTTLayer '{}'", mTextureName, mName));
    }
}

void RTTLayer::createRttTexture()
{
    ROR_ASSERT(!mTexture && "RTTLayer texture already exists; destroy it first before creating a new one");
    bool sizeIsValid = mTextureSize.width > 0 && mTextureSize.height > 0;
    if (!mTexture && sizeIsValid)
    {
        std::string name = mTextureName.empty() ? MyGUI::utility::toString((size_t)this, getClassTypeName()) : mTextureName;
        mTexture = MyGUI::RenderManager::getInstance().createTexture(name);
        mTexture->createManual(mTextureSize.width, mTextureSize.height, MyGUI::TextureUsage::RenderTarget, MyGUI::PixelFormat::R8G8B8A8);
        mOutOfDate = true;
        LOG(fmt::format("[RoR|Dashboard] Created texture '{}' in RTTLayer '{}'", mTextureName, mName));
    }
}

// ----------------------- RTTLayerManager -----------------------

RTTLayer* RTTLayerManager::CreateOrReuseRttLayer()
{
    RTTLayer* rttLayer = nullptr;
    if (mReusableRttLayers.size() > 0)
    {
        rttLayer = mReusableRttLayers.back();
        mReusableRttLayers.pop_back();
        LOG(fmt::format("[RoR|Dashboard] Reusing RTTLayer '{}'", rttLayer->getName()));
    }
    else
    {
        MyGUI::ILayer* layer = MyGUI::LayerManager::getInstance().createLayerAt(fmt::format("RttLayer_{}", mNextRenderdashRttNum++), "RTTLayer", 0);
        rttLayer = dynamic_cast<RTTLayer*>(layer);
        LOG(fmt::format("[RoR|Dashboard] Creating new RTTLayer '{}'", rttLayer->getName()));
    }
    return rttLayer;
}

void RTTLayerManager::RecycleRttLayer(RTTLayer* rttLayer)
{
    LOG(fmt::format("[RoR|Dashboard] Recycling RTTLayer '{}'", rttLayer->getName()));
    mReusableRttLayers.push_back(rttLayer);
}
