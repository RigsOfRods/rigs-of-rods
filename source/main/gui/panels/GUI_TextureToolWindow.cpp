/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer

    For more information, see http://www.rigsofrods.org/

    Rigs of Rods is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3, as
    published by the Free Software Foundation.

    Rigs of Rods is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Rigs of Rods. If not, see <http://www.gnu.org/licenses/>.
*/


#include "GUI_TextureToolWindow.h"

#include <Ogre.h>

#include "Application.h"
#include "GUI_GameConsole.h"
#include "Language.h"
#include "PlatformUtils.h"
#include "Utils.h"

namespace RoR {
namespace GUI {

using namespace Ogre;

#define MAIN_WIDGET ((MyGUI::Window*)mMainWidget)

TextureToolWindow::TextureToolWindow()
{
    initialiseByAttributes(this);

    ((MyGUI::Window*)mMainWidget)->setCaption(_L("Texture Tool"));
    ((MyGUI::Window*)mMainWidget)->eventWindowButtonPressed += MyGUI::newDelegate(this, &TextureToolWindow::notifyWindowPressed);

    mBtnSavePNG->setCaption(_L("Save as PNG"));
    mBtnSavePNG->eventMouseButtonClick += MyGUI::newDelegate(this, &TextureToolWindow::eventClickSavePNGButton);

    mBtnSaveRAW->setCaption(_L("Save Raw"));
    mBtnSaveRAW->eventMouseButtonClick += MyGUI::newDelegate(this, &TextureToolWindow::eventClickSaveRAWButton);

    mCBo->eventComboAccept += MyGUI::newDelegate(this, &TextureToolWindow::eventSelectTexture);
    mCBo->eventComboChangePosition += MyGUI::newDelegate(this, &TextureToolWindow::eventSelectTexture2);

    mChkDynamic->eventMouseButtonClick += MyGUI::newDelegate(this, &TextureToolWindow::eventClickDynamicButton);
    mChkDynamic->setStateSelected(true);

    MAIN_WIDGET->setVisible(false);
}

void TextureToolWindow::SetVisible(bool v)
{
    MAIN_WIDGET->setVisible(v);
    if (v)
    {
        fillCombo();
    }
}

bool TextureToolWindow::IsVisible()
{
    return MAIN_WIDGET->getVisible();
}

void TextureToolWindow::fillCombo()
{
    bool dynamicOnly = mChkDynamic->getStateSelected();
    mCBo->removeAllItems();

    ResourceManager::ResourceMapIterator it = TextureManager::getSingleton().getResourceIterator();

    std::vector<std::pair<String, String>> texture_names;
    while (it.hasMoreElements())
    {
        TexturePtr txt(it.getNext().staticCast<Texture>());

        if (dynamicOnly && ((txt->getUsage() & TU_STATIC) != 0))
            continue;

        String name = txt->getName();
        StringUtil::toLowerCase(name);
        texture_names.push_back({name, txt->getName()});
    }

    std::sort(texture_names.begin(), texture_names.end());

    for (auto name : texture_names)
    {
        mCBo->addItem(name.second);
    }

    if (mCBo->getItemCount() > 0)
    {
        int index = 0;
        const auto it = std::find_if(texture_names.begin(), texture_names.end(),
                [](const std::pair<std::string, std::string>& t) { return t.second.substr(0, 9) == "MapRttTex"; });
        if (it != texture_names.end())
        {
            index = std::distance(texture_names.begin(), it);
        }
        mCBo->setIndexSelected(index);
        mCBo->beginToItemSelected();
        updateControls(mCBo->getItemNameAt(index));
    }
}

void TextureToolWindow::saveTexture(String texName, bool usePNG)
{
    try
    {
        TexturePtr tex = TextureManager::getSingleton().getByName(texName);
        if (tex.isNull())
            return;

        Image img;
        tex->convertToImage(img);

        // Save to disk!
        String outname = PathCombine(App::sys_user_dir.GetActive(), StringUtil::replaceAll(texName, "/", "_"));
        if (usePNG)
            outname += ".png";

        img.save(outname);

        UTFString msg = _L("saved texture as ") + outname;
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_MSGTYPE_INFO, msg, "information.png");
    }
    catch (Exception& e)
    {
        UTFString str = "Exception while saving image: " + e.getFullDescription();
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_MSGTYPE_INFO, str, "error.png");
    }
}

void TextureToolWindow::updateControls(String texName)
{
    try
    {
        bool exists = TextureManager::getSingleton().resourceExists(texName);
        if (!exists)
        {
            mTxt->setCaption(convertToMyGUIString("Texture not found:\n" + texName));
            mBtnSavePNG->setEnabled(false);
            return;
        }

        TexturePtr tex = TextureManager::getSingleton().getByName(texName);
        if (tex.isNull())
        {
            mTxt->setCaption(convertToMyGUIString("Error loading texture:\n" + texName));
            mBtnSavePNG->setEnabled(false);
            return;
        }

        String str = "#aa0000" + texName + "#000000\n";
        str += "#00aa00res: #000000" + TOSTRING(tex->getWidth()) + " x " + TOSTRING(tex->getHeight()) + " pixels\n";
        str += "#00aa00size: #000000" + formatBytes(tex->getSize()) + "\n";
        str += "#00aa00format: #000000" + PixelUtil::getFormatName(tex->getFormat()) + "\n";
        if (tex->getNumFaces() > 1)
            str += "#00aa00faces: #000000" + TOSTRING(tex->getNumFaces()) + "\n";
        if (tex->getFSAA() > 0)
            str += "#00aa00FSAA: #000000" + TOSTRING(tex->getFSAA()) + "\n";
        if (tex->getNumMipmaps() > 0)
            str += "#00aa00mipmaps: #000000" + TOSTRING(tex->getNumMipmaps()) + "\n";

        String typeStr = "";
        switch (tex->getTextureType())
        {
        case TEX_TYPE_1D: typeStr = "1D";
            break;
        case TEX_TYPE_2D: typeStr = "2D";
            break;
        case TEX_TYPE_3D: typeStr = "3D";
            break;
        case TEX_TYPE_CUBE_MAP: typeStr = "Cube Map";
            break;
        }
        str += "#00aa00type: #000000" + typeStr + "\n";

        String usageStr = "";
        if (tex->getUsage() & TU_STATIC)
            usageStr += "static,\n";
        if (tex->getUsage() & TU_DYNAMIC)
            usageStr += "dynamic,\n";
        if (tex->getUsage() & TU_WRITE_ONLY)
            usageStr += "write only,\n";
        if (tex->getUsage() & TU_STATIC_WRITE_ONLY)
            usageStr += "static write only,\n";
        if (tex->getUsage() & TU_DYNAMIC_WRITE_ONLY)
            usageStr += "dynamic write only,\n";
        if (tex->getUsage() & TU_DYNAMIC_WRITE_ONLY_DISCARDABLE)
            usageStr += "dynamic write only discardable,\n";
        if (tex->getUsage() & TU_AUTOMIPMAP)
            usageStr += "automipmap,\n";
        if (tex->getUsage() & TU_RENDERTARGET)
            usageStr += "rendertarget,\n";
        if (tex->getUsage() & TU_DEFAULT)
            usageStr += "default\n";

        str += "#00aa00usage: #000000" + usageStr + "\n";
        if (tex->getDepth() > 1)
            str += "#00aa00depth: #000000" + TOSTRING(tex->getDepth()) + "\n";

        mTxt->setCaption(convertToMyGUIString(str));
        mImage->setImageTexture(texName);
        mBtnSavePNG->setEnabled(true);
    }
    catch (Exception& e)
    {
        UTFString str = "Exception while opening texture:" + e.getFullDescription();
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_MSGTYPE_INFO, str, "error.png");
    }
}

void TextureToolWindow::eventSelectTexture2(MyGUI::ComboBoxPtr _sender, size_t _index)
{
    updateControls(mCBo->getItemNameAt(_index));
}

void TextureToolWindow::eventClickDynamicButton(MyGUI::WidgetPtr _sender)
{
    mChkDynamic->setStateSelected(!mChkDynamic->getStateSelected());
    fillCombo();
}

void TextureToolWindow::notifyWindowPressed(MyGUI::Window* _widget, const std::string& _name)
{
    if (_name == "close")
    {
        SetVisible(false);
    }
}

void TextureToolWindow::eventClickSavePNGButton(MyGUI::WidgetPtr _sender)
{
    saveTexture(mCBo->getItemNameAt(mCBo->getIndexSelected()), true);
}

void TextureToolWindow::eventClickSaveRAWButton(MyGUI::WidgetPtr _sender)
{
    saveTexture(mCBo->getItemNameAt(mCBo->getIndexSelected()), false);
}

void TextureToolWindow::eventSelectTexture(MyGUI::WidgetPtr _sender)
{
    updateControls(mCBo->getItemNameAt(mCBo->getIndexSelected()));
}

} // namespace GUI
} // namespace RoR
