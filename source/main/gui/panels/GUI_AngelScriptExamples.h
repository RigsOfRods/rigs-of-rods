/*
    This source file is part of Rigs of Rods
    Copyright 2021 tritonas00
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

/// @file

#pragma once

#include "Application.h"
#include "GUI_ConsoleView.h"


namespace RoR {

class AngelScriptExamples
{
public:
    void Draw();
private:
    void DrawRowSlider(const char* nameStr, std::string codeStr, const char* descStr, float min, float max, float &var_ref);
    void DrawRowText(const char* nameStr, std::string codeStr, const char* descStr);
    void DrawRowCheckbox(const char* nameStr, std::string codeStr, const char* descStr, bool &var_ref, const char* label);
    void DrawRowInt(const char* nameStr, std::string codeStr, const char* descStr, int &var_ref);
    void DrawRowIntCheckbox(const char* nameStr, std::string codeStr, const char* descStr, int &var_ref, bool &on, const char* label);
    void DrawRowIntNode(const char* nameStr, std::string codeStr, const char* descStr, int &var_ref, int &node_x, int &node_y, int &node_z);

    float scale = 1.f;
    float mass = 1000.f;
    bool reset = false;
    bool locked = false;
    int light = 1;
    int blink = 1;
    int node = 1;
    bool visible = false;
    int custom_light = 1;
};

} // namespace RoR