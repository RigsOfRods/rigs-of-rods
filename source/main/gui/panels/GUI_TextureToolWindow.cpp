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
#include "Console.h"
#include "GUIManager.h"
#include "Language.h"
#include "OgreImGui.h"
#include "PlatformUtils.h"
#include "Utils.h"

namespace RoR {
namespace GUI {

void TextureToolWindow::Draw()
{
    ImGui::SetNextWindowPosCenter(ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, 550.f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_L("Texture Tool"), &m_is_visible))
    {
        ImGui::End(); // The window is collapsed
        return;
    }

    // left
    ImGui::BeginGroup();

    ImGui::Checkbox(_L("Dynamic only"), &m_show_dynamic_only);

    ImGui::BeginChild("texture list", ImVec2(LEFT_PANE_WIDTH, 0), true);
    auto itor = Ogre::TextureManager::getSingleton().getResourceIterator();
    while (itor.hasMoreElements())
    {
        Ogre::TexturePtr tex = Ogre::static_pointer_cast<Ogre::Texture>(itor.getNext());
        if (m_show_dynamic_only && ((tex->getUsage() & Ogre::TU_STATIC) != 0))
        {
            continue;
        }

        bool selected = m_display_tex == tex;
        if (ImGui::Selectable(tex->getName().c_str(), &selected))
        {
            m_display_tex = tex;
        }
    }
    ImGui::EndChild(); // texture list
    ImGui::EndGroup(); // left
    ImGui::SameLine();

    // right
    ImGui::BeginGroup();

    if (m_display_tex)
    {
        ImGui::Text("%s", m_display_tex->getName().c_str());
        ImGui::Separator();

        // Draw the image
        float max_width = ImGui::GetWindowSize().x -
            (LEFT_PANE_WIDTH + ImGui::GetStyle().ItemSpacing.x + 2*ImGui::GetStyle().WindowPadding.x);
        float max_height = ImGui::GetWindowSize().y * 0.5;
        ImVec2 size(m_display_tex->getWidth(), m_display_tex->getHeight());
        size *= max_width / size.x; // Fit size along X
        if (size.y > max_height) // Reduce size along Y if needed
        {
            size *= max_height / size.y;
        }
        ImGui::Image(reinterpret_cast<ImTextureID>(m_display_tex->getHandle()), size);

        // Draw image info
        ImGui::BeginChild("tex info", ImVec2(0, -ImGui::GetItemsLineHeightWithSpacing())); // Leave room for 1 line below us

        ImGui::Text("Res: %u x %u pixels", m_display_tex->getWidth(), m_display_tex->getHeight());
        ImGui::Text("Size: %s", formatBytes(m_display_tex->getSize()).asUTF8_c_str());
        ImGui::Text("Format: %s", Ogre::PixelUtil::getFormatName(m_display_tex->getFormat()));
        if (m_display_tex->getNumFaces() > 1)
            ImGui::Text("Num. faces: %u", m_display_tex->getNumFaces());
        if (m_display_tex->getFSAA() > 0)
            ImGui::Text("FSAA: %u", m_display_tex->getFSAA());
        if (m_display_tex->getNumMipmaps() > 0)
            ImGui::Text("Num. mipmaps: %u", m_display_tex->getNumMipmaps());

        static const char* tex_type_names[] = 
        { "~invalid~", "1D", "2D", "3D", "Cubemap", "2D array", "2D rect", "External OES" };
        ImGui::Text("Type: %s", tex_type_names[m_display_tex->getTextureType()]);

        std::string usageStr = "";
        if (m_display_tex->getUsage() & Ogre::TU_STATIC)
            usageStr += "static,\n";
        if (m_display_tex->getUsage() & Ogre::TU_DYNAMIC)
            usageStr += "dynamic,\n";
        if (m_display_tex->getUsage() & Ogre::TU_WRITE_ONLY)
            usageStr += "write only,\n";
        if (m_display_tex->getUsage() & Ogre::TU_STATIC_WRITE_ONLY)
            usageStr += "static write only,\n";
        if (m_display_tex->getUsage() & Ogre::TU_DYNAMIC_WRITE_ONLY)
            usageStr += "dynamic write only,\n";
        if (m_display_tex->getUsage() & Ogre::TU_DYNAMIC_WRITE_ONLY_DISCARDABLE)
            usageStr += "dynamic write only discardable,\n";
        if (m_display_tex->getUsage() & Ogre::TU_AUTOMIPMAP)
            usageStr += "automipmap,\n";
        if (m_display_tex->getUsage() & Ogre::TU_RENDERTARGET)
            usageStr += "rendertarget,\n";
        if (m_display_tex->getUsage() & Ogre::TU_DEFAULT)
            usageStr += "default\n";
        ImGui::TextWrapped("Usage: %s", usageStr.c_str());

        ImGui::Text("Depth: %u", m_display_tex->getDepth());
        ImGui::EndChild(); // tex info

        ImGui::BeginChild("buttons");
        if (ImGui::Button(_L("Save as PNG")))
        {
            this->SaveTexture(m_display_tex->getName(), /*usePNG =*/ true);
        }
        ImGui::SameLine();
        if (ImGui::Button(_L("Save Raw")))
        {
            this->SaveTexture(m_display_tex->getName(), /*usePNG =*/ false);
        }
        ImGui::EndChild(); // buttons
    }

    ImGui::EndGroup(); // right

    App::GetGuiManager()->RequestGuiCaptureKeyboard(ImGui::IsWindowHovered());
    ImGui::End();
}

void TextureToolWindow::SaveTexture(std::string texName, bool usePNG)
{
    using namespace Ogre;
    try
    {
        TexturePtr tex = TextureManager::getSingleton().getByName(texName);
        if (tex.isNull())
            return;

        Image img;
        tex->convertToImage(img);

        // Save to disk!
        std::string outname = PathCombine(App::sys_user_dir->GetActiveStr(), StringUtil::replaceAll(texName, "/", "_"));
        if (usePNG)
            outname += ".png";

        img.save(outname);

        std::string msg = _L("saved texture as ") + outname;
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_NOTICE, msg, "information.png");
    }
    catch (Exception& e)
    {
        std::string str = "Exception while saving image: " + e.getFullDescription();
        RoR::App::GetConsole()->putMessage(Console::CONSOLE_MSGTYPE_INFO, Console::CONSOLE_SYSTEM_ERROR, str, "error.png");
    }
}

} // namespace GUI
} // namespace RoR
