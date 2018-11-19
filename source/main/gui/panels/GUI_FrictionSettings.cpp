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

/// @file
/// @author Thomas Fischer (thomas{AT}thomasfischer{DOT}biz)
/// @date   7th of September 2009


#include "GUI_FrictionSettings.h"

#include "Application.h"
#include "BeamData.h"
#include "Collisions.h"
#include "GUIManager.h"
#include "Language.h"
#include "Utils.h"

namespace RoR {
namespace GUI {

using namespace Ogre;

FrictionSettings::FrictionSettings() :
    col(0)
    , active_gm(0)
    , selected_gm(0)
    , win(0)
{
    int x = 0, y = 0, by = 0;
    MyGUI::EditPtr e;
    MyGUI::ScrollBar* h;
    MyGUI::ButtonPtr b;
    MyGUI::TextBox* t;

    msgwin = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 300, MyGUI::Align::Center, "Overlapped");
    msgwin->setCaption(_L("Friction Help"));
    msgwin->eventWindowButtonPressed += MyGUI::newDelegate(this, &FrictionSettings::notifyHelpWindowButtonPressed);
    e = msgwin->createWidget<MyGUI::Edit>("EditBoxStretch", 0, 0, 400, 300, MyGUI::Align::Default, "helptext");
    e->setCaption("");
    e->setEditWordWrap(true);
    e->setEditStatic(true);
    msgwin->setVisible(false);

    win = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCSX", 0, 0, 400, 500, MyGUI::Align::Center, "Overlapped");
    win->setCaption(_L("Friction Settings"));

    // active ground
    x = 10 , y = 10;
    t = win->createWidget<MyGUI::TextBox>("TextBox", x, y, 270, 20, MyGUI::Align::Default, "text_current_ground");
    x += 275;
    t->setCaption("Current active Ground: n/a");
    t->setFontHeight(18);
    b = win->createWidget<MyGUI::Button>("Button", x, y, 70, 20, MyGUI::Align::Default, "select_current_ground");
    x += 75;
    b->setCaption("select");
    b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);

    // combo box
    x = 20;
    y += 20;
    t = win->createWidget<MyGUI::TextBox>("TextBox", x, y, 170, 20, MyGUI::Align::Default);
    x += 175;
    t->setCaption(_L("selected Ground Type:"));
    t->setTextAlign(MyGUI::Align::Right);

    MyGUI::ComboBoxPtr cb = win->createWidget<MyGUI::ComboBox>("ComboBox", x, y, 115, 20, MyGUI::Align::Default, "combo_grounds");
    cb->eventComboAccept += MyGUI::newDelegate(this, &FrictionSettings::event_combo_grounds_eventComboAccept);
    cb->eventComboChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_combo_grounds_eventComboAccept);
    cb->setEditStatic(true);
    // ground models are not loaded here yet, so we will add them once we show the dialog

    x = 10;
    y += 30;
    t = win->createWidget<MyGUI::TextBox>("TextBox", x, y, 170, 20, MyGUI::Align::Default);
    x += 175;
    t->setCaption(_L("Solid ground settings"));

    // adding panel for solid ground
    x = 10;
    by = y + 20; // set box start now
    MyGUI::WidgetPtr p = win->createWidget<MyGUI::Widget>("PanelSkin", x, by, 370, 25, MyGUI::Align::Default);
    {
        // solid_level
        int lx = 0, ly = 5; // local coordinate system
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Solid ground level:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_level");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_level_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_level_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["solid_level"] = std::pair<UTFString, UTFString>(_L("Solid ground level"), _L("With this you can define how deep the solid ground is. If it is 0 then the surface will be solid. If it is 0.1 then you'll have 10 cm of fluid on top of solid ground. If it is 100 then the solid ground will be way deep (100m), with fluid on top."));
        minMaxs["solid_level"] = std::pair<Real, Real>(0, 200);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_level_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // solid_strength
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Strength:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_strength");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_strength_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_strength_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["solid_strength"] = std::pair<UTFString, UTFString>(_L("Strength"), _L("This parameter raises or diminishes surface friction in a generic way. It is here so as to be able to do quick calibrations of friction. Start with having this to 1.0 and after tuning the rest of the surface variables, come back and play with this."));
        minMaxs["solid_strength"] = std::pair<Real, Real>(0, 2);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_strength_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // solid_static_friction
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Static friction coef:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_static_friction");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_static_friction_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_static_friction_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["solid_static_friction"] = std::pair<UTFString, UTFString>(_L("Static friction coef"), _L("Static friction keeps you in the same place when you are stopped on a hill. In the real world this friction is always bigger than dynamic friction (sliding friction). Start with 0.5 and work from there. It is better to try to find some experimentally validated values for this and the rest of surface friction variables in the net, and then to fine tune via strength."));
        minMaxs["solid_static_friction"] = std::pair<Real, Real>(0.1f, 2);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_static_friction_help");
        lx += 20;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // solid_adhension_velo
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Adhesion velocity:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_adhension_velo");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_adhension_velo_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_adhension_velo_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["solid_adhension_velo"] = std::pair<UTFString, UTFString>(_L("Adhesion velocity"), _L("Below this value static friction laws apply, above it dynamic friction laws apply. Value should be small, in the range of 0.1-0.4 . This velocity threshold is also used by fluid dynamics so you should always define it. NEVER set it to 0."));
        minMaxs["solid_adhension_velo"] = std::pair<Real, Real>(0.1f, 0.5f);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_adhension_velo_help");
        lx += 20;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // solid_dynamic_friction
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Dynamic friction coef:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_dynamic_friction");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_dynamic_friction_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_dynamic_friction_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["solid_dynamic_friction"] = std::pair<UTFString, UTFString>(_L("Dynamic friction coef"), _L("Or sliding friction coef. It should be smaller than static friction coef. This parameter defines how much friction you'll have when sliding. Try to find some values for it from the net."));
        minMaxs["solid_dynamic_friction"] = std::pair<Real, Real>(0.1f, 1.5f);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_dynamic_friction_help");
        lx += 20;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // solid_hydrodynamic
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Hydrodynamic friction coef:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_hydrodynamic");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_hydrodynamic_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_hydrodynamic_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["solid_hydrodynamic"] = std::pair<UTFString, UTFString>(_L("Hydrodynamic friction coef"), _L("This friction defines the added friction that you'll feel from a surface that has a little film of fluid on it. It is kind of redundant with all the fluid physics below, but it is here so as for experimentally validated values from the net to be usable. If you decide that you'll simulate the film of fluid with the more complex fluid physics below, then just set this to 0."));
        minMaxs["solid_hydrodynamic"] = std::pair<Real, Real>(0, 1.5f);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_hydrodynamic_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // solid_stribeck
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Stribeck velocity:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_stribeck");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_stribeck_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_stribeck_scroll");
        lx += 65;
        helpTexts["solid_stribeck"] = std::pair<UTFString, UTFString>(_L("Stribeck velocity"), _L("You'll either find stribeck velocity in the net, or the inverse (1/stribeck velocity) of it described as 'stribeck coef'. It defines the shape of the dynamic friction curve. Lets leave it at that. Just find some nice values for it from the net."));
        minMaxs["solid_stribeck"] = std::pair<Real, Real>(0, 1000);
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_stribeck_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // solid_alpha
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("alpha:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "solid_alpha");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "solid_alpha_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "solid_alpha_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["solid_alpha"] = std::pair<UTFString, UTFString>(_L("Alpha"), _L("Its usual value is 2. But you can try others."));
        minMaxs["solid_alpha"] = std::pair<Real, Real>(0, 200);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "solid_alpha_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // combo_fx_type
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("fx_type:"));
        t->setTextAlign(MyGUI::Align::Right);
        cb = p->createWidget<MyGUI::ComboBox>("ComboBox", lx, ly, 115, 20, MyGUI::Align::Default, "combo_fx_type");
        lx += 120;
        cb->addItem("dusty");
        cb->addItem("hard");
        cb->addItem("clumpy");
        cb->setEditStatic(true);
        cb->setIndexSelected(0);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "combo_fx_type_edited");
        lx += 20;
        helpTexts["combo_fx"] = std::pair<UTFString, UTFString>(_L("FX Type"), _L("The type of special effects that RoR will use to give the appearance of a surface. It doesn't affect the physics at all"));
        minMaxs["combo_fx"] = std::pair<Real, Real>(0, 0);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "combo_fx_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // fx_color
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("fx_color:"));
        t->setTextAlign(MyGUI::Align::Right);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 140, 20, MyGUI::Align::Default, "fx_color_text");
        lx += 145;
        t->setCaption("");
        t->setTextAlign(MyGUI::Align::Left);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "fx_color_text_edited");
        lx += 20;
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "fx_color_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");
        helpTexts["fx_color"] = std::pair<UTFString, UTFString>(_L("FX Colour"), _L("The color of RoR's special effects"));
        minMaxs["fx_color"] = std::pair<Real, Real>(0, 0);
        // TO BE DONE

        lx = 0;
        ly += 20;
        // add height to global height
        y += ly;
    }
    p->setSize(370, y - by + 25);

    x = 10;
    y += 40;
    t = win->createWidget<MyGUI::TextBox>("TextBox", x, y, 170, 20, MyGUI::Align::Default);
    x += 175;
    t->setCaption(_L("Fluid Settings"));

    // adding panel for solid ground
    x = 10;
    by = y + 20; // set box start now
    p = win->createWidget<MyGUI::Widget>("PanelSkin", x, by, 370, 25, MyGUI::Align::Default);
    {
        int lx = 0, ly = 5; // local coordinate system

        // fluid_flowbeh
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Flow behavior index:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "fluid_flowbeh");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_flowbeh_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "fluid_flowbeh_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["fluid_flowbeh"] = std::pair<UTFString, UTFString>(_L("Flow behavior index"), _L("If it is 1.0 then the fluid will behave like water. The lower you get from 1.0, the more like mud the fluid will behave, meaning that for small velocities the fluid will resist motion and for large velocities the fluid will not resist so much. The higher you get from 1.0 the more like sand the fluid will behave. The bigger the velocity, the bigger the resistance of the fluid (try to hit sand hard it'll feel like stone)."));
        minMaxs["fluid_flowbeh"] = std::pair<Real, Real>(-2, 2);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_flowbeh_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // fluid_flowcon
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Flow consistency:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "fluid_flowcon");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_flowcon_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "fluid_flowcon_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["fluid_flowcon"] = std::pair<UTFString, UTFString>(_L("Flow consistency"), _L("Think of it as default fluid resistance. Behavior index above changes it at real time. Useful values in practice are quite large."));
        minMaxs["fluid_flowcon"] = std::pair<Real, Real>(10, 100000);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_flowcon_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // fluid_density
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Fluid density:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "fluid_density");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_density_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "fluid_density_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["fluid_density"] = std::pair<UTFString, UTFString>(_L("Fluid density"), _L("In mud (or sand) the resistance of the fluid described by the parameters above will stop you and so keep you from sinking. But for substances like water it isn't the drag that stops you from sinking. Its buoyancy. This parameter is here so as to keep you from sinking when you wish to simulate fluids with low drag (resistance). For fluids like mud or sand you can put it at 0, but it is best to keep it at some minimum value. For fluids with behavior index >=1 it will behave like you are in water. For fluids with behavior index <1 it'll behave like you are in mud."));
        minMaxs["fluid_density"] = std::pair<Real, Real>(10, 100000);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_density_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        // fluid_drag_anisotropy
        lx = 0;
        ly += 20;
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 170, 20, MyGUI::Align::Default);
        lx += 175;
        t->setCaption(_L("Drag anisotropy:"));
        t->setTextAlign(MyGUI::Align::Right);
        e = p->createWidget<MyGUI::Edit>("EditBox", lx, ly, 80, 20, MyGUI::Align::Default, "fluid_drag_anisotropy");
        lx += 85;
        e->eventEditTextChange += MyGUI::newDelegate(this, &FrictionSettings::event_edit_TextChange);
        t = p->createWidget<MyGUI::TextBox>("TextBox", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_drag_anisotropy_edited");
        lx += 20;
        h = p->createWidget<MyGUI::ScrollBar>("ScrollBarH", lx, ly, 60, 20, MyGUI::Align::Default, "fluid_drag_anisotropy_scroll");
        lx += 65;
        h->eventScrollChangePosition += MyGUI::newDelegate(this, &FrictionSettings::event_scroll_value);
        h->setScrollRange(1000);
        helpTexts["fluid_drag_anisotropy"] = std::pair<UTFString, UTFString>(_L("Drag anisotropy"), _L("This parameter is for making it easier(cheating) to get out from mud. To get stuck in real mud isn't fun at all, so this makes the mud push up. Ranges in this parameter are from 0 to 1 . If you set it at 1 then you'll get real mud. For values from 0 to 1, the behavior goes from real mud to easy mud depending on this parameter and the value of Adhesion velocity. For velocity 0 real mud it is. For velocity >= adhesion velocity easy mud it is."));
        minMaxs["fluid_drag_anisotropy"] = std::pair<Real, Real>(0, 1);
        b = p->createWidget<MyGUI::Button>("Button", lx, ly, 20, 20, MyGUI::Align::Default, "fluid_drag_anisotropy_help");
        lx += 25;
        b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
        b->setCaption("?");

        lx = 0;
        ly += 20;
        // add height to global height
        y += ly;
    }
    p->setSize(370, y - by + 25);

    x = 10;
    y += 40;
    b = win->createWidget<MyGUI::Button>("Button", x, y, 350, 30, MyGUI::Align::Default, "apply_changes");
    x += 25;
    b->eventMouseButtonClick += MyGUI::newDelegate(this, &FrictionSettings::event_btn_MouseButtonClick);
    b->setCaption(_L("Apply Changes"));

    /*
    x=10; y+=20;
    t = win->createWidget<MyGUI::TextBox>("TextBox",  x, y, 170, 20,  MyGUI::Align::Default); x+=175;
    t->setCaption(_L("Ground Debug"));
    */

    win->eventWindowButtonPressed += MyGUI::newDelegate(this, &FrictionSettings::notifyWindowButtonPressed);
    win->setVisible(false);
}

FrictionSettings::~FrictionSettings()
{
}

void FrictionSettings::setShaded(bool value)
{
    if (!win || !win->getVisible())
        return;
    if (value)
    {
        MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerFadeAlpha::getClassTypeName());
        MyGUI::ControllerFadeAlpha* controller = item->castType<MyGUI::ControllerFadeAlpha>();
        controller->setAlpha(0.9f);
        controller->setCoef(3.0f);
        controller->setEnabled(true);
        MyGUI::ControllerManager::getInstance().addItem(win, controller);
    }
    else
    {
        MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerFadeAlpha::getClassTypeName());
        MyGUI::ControllerFadeAlpha* controller = item->castType<MyGUI::ControllerFadeAlpha>();
        controller->setAlpha(0.2f);
        controller->setCoef(3.0f);
        controller->setEnabled(true);
        MyGUI::ControllerManager::getInstance().addItem(win, controller);
    }
}

void FrictionSettings::SetVisible(bool value)
{
    if (!col)
        return;
    if (value)
    {
        // upon show, refresh list
        MyGUI::ComboBoxPtr cb = (MyGUI::ComboBoxPtr)win->findWidget("combo_grounds");
        cb->removeAllItems();

        std::map<String, ground_model_t>::iterator it;
        std::map<String, ground_model_t>* gmm = col->getGroundModels();
        for (it = gmm->begin(); it != gmm->end(); it++)
        {
            cb->addItem((*it).second.name);
        }
        cb->setEditStatic(true);
        cb->setIndexSelected(0);
        ground_model_t* gm = col->getGroundModelByString(cb->getItemNameAt(0));
        selected_gm = gm;
        if (gm)
            updateControls(gm, false);
    }
    else
    {
        RoR::App::GetGuiManager()->UnfocusGui();
    }
    win->setVisibleSmooth(value);
}

bool FrictionSettings::IsVisible()
{
    return win->getVisible();
}

void FrictionSettings::setActiveCol(ground_model_t* gm)
{
    if (!gm)
        return;
    if (active_gm == gm)
        return; //only set once
    active_gm = gm;

    UTFString tmp = _L("Current active Ground: ") + ANSI_TO_UTF(gm->name);
    MyGUI::TextBox* t = (MyGUI::TextBox*)win->findWidget("text_current_ground");
    if (t)
        t->setCaption(convertToMyGUIString(tmp));
    if (selected_gm == gm)
        updateControls(gm);
}

void FrictionSettings::updateControls(ground_model_t* gm, bool setCombo)
{
    if (!gm)
        return;

    if (setCombo)
    {
        MyGUI::ComboBoxPtr cb = (MyGUI::ComboBoxPtr)win->findWidget("combo_grounds");
        if (!cb)
            return;

        for (int i = 0; i < (int)cb->getItemCount(); i++)
        {
            if (cb->getItemNameAt(i) == MyGUI::UString(gm->name))
            {
                cb->setIndexSelected(i);
                selected_gm = gm;
                break;
            }
        }
    }

    MyGUI::EditPtr e = (MyGUI::EditPtr)win->findWidget("solid_adhension_velo");
    if (e)
        e->setCaption(TOSTRING(gm->va));
    MyGUI::ScrollBar* h = (MyGUI::ScrollBar *)win->findWidget("solid_adhension_velo_scroll");
    MyGUI::TextBox* edt = (MyGUI::TextBox *)win->findWidget("solid_adhension_velo_edited");
    if (edt)
        edt->setCaption("");
    std::pair<Real, Real> f = minMaxs["solid_adhension_velo"];
    size_t fa = ((gm->va + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("solid_level");
    if (e)
        e->setCaption(TOSTRING(gm->solid_ground_level));
    h = (MyGUI::ScrollBar *)win->findWidget("solid_adhension_velo_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("solid_level_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["solid_level"];
    fa = ((gm->solid_ground_level + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("solid_static_friction");
    if (e)
        e->setCaption(TOSTRING(gm->ms));
    h = (MyGUI::ScrollBar *)win->findWidget("solid_static_friction_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("solid_static_friction_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["solid_static_friction"];
    fa = ((gm->ms + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("solid_dynamic_friction");
    if (e)
        e->setCaption(TOSTRING(gm->mc));
    h = (MyGUI::ScrollBar *)win->findWidget("solid_dynamic_friction_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("solid_dynamic_friction_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["solid_dynamic_friction"];
    fa = ((gm->mc + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("solid_hydrodynamic");
    if (e)
        e->setCaption(TOSTRING(gm->t2));
    h = (MyGUI::ScrollBar *)win->findWidget("solid_hydrodynamic_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("solid_hydrodynamic_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["solid_hydrodynamic"];
    fa = ((gm->t2 + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("solid_stribeck");
    if (e)
        e->setCaption(TOSTRING(gm->vs));
    h = (MyGUI::ScrollBar *)win->findWidget("solid_stribeck_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("solid_stribeck_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["solid_stribeck"];
    fa = ((gm->vs + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("solid_alpha");
    if (e)
        e->setCaption(TOSTRING(gm->alpha));
    h = (MyGUI::ScrollBar *)win->findWidget("solid_alpha_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("solid_alpha_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["solid_alpha"];
    fa = ((gm->alpha + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("solid_strength");
    if (e)
        e->setCaption(TOSTRING(gm->strength));
    h = (MyGUI::ScrollBar *)win->findWidget("solid_strength_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("solid_strength_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["solid_strength"];
    fa = ((gm->strength + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    // fluid
    e = (MyGUI::EditPtr)win->findWidget("fluid_flowbeh");
    if (e)
        e->setCaption(TOSTRING(gm->flow_behavior_index));
    h = (MyGUI::ScrollBar *)win->findWidget("fluid_flowbeh_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("fluid_flowbeh_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["fluid_flowbeh"];
    fa = ((gm->flow_behavior_index + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("fluid_flowcon");
    if (e)
        e->setCaption(TOSTRING(gm->flow_consistency_index));
    h = (MyGUI::ScrollBar *)win->findWidget("fluid_flowcon_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("fluid_flowcon_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["fluid_flowcon"];
    fa = ((gm->flow_consistency_index + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("fluid_density");
    if (e)
        e->setCaption(TOSTRING(gm->fluid_density));
    h = (MyGUI::ScrollBar *)win->findWidget("fluid_density_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("fluid_density_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["fluid_density"];
    fa = ((gm->fluid_density + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

    e = (MyGUI::EditPtr)win->findWidget("fluid_drag_anisotropy");
    if (e)
        e->setCaption(TOSTRING(gm->drag_anisotropy));
    h = (MyGUI::ScrollBar *)win->findWidget("fluid_drag_anisotropy_scroll");
    edt = (MyGUI::TextBox *)win->findWidget("fluid_drag_anisotropy_edited");
    if (edt)
        edt->setCaption("");
    f = minMaxs["fluid_drag_anisotropy"];
    fa = ((gm->drag_anisotropy + f.first) / (f.second - f.first)) * h->getScrollRange();
    if (h)
        h->setScrollPosition(fa);

}

void FrictionSettings::event_combo_grounds_eventComboAccept(MyGUI::ComboBoxPtr _sender, size_t _index)
{
    if (!col)
        return;
    if (!win->getVisible())
        return;
    MyGUI::ComboBoxPtr cb = (MyGUI::ComboBoxPtr)win->findWidget("combo_grounds");
    if (!cb)
        return;
    ground_model_t* gm = col->getGroundModelByString(cb->getItemNameAt(_index).asUTF8_c_str());
    if (gm)
        updateControls(gm, false);
    selected_gm = gm;
}

void FrictionSettings::event_edit_TextChange(MyGUI::EditPtr _sender)
{
    String name = _sender->getName();
    MyGUI::TextBox* edt = (MyGUI::TextBox *)win->findWidget(name + "_edited");
    if (edt)
        edt->setCaption("!!");
}

void FrictionSettings::event_btn_MouseButtonClick(MyGUI::WidgetPtr _sender)
{
    String name = _sender->getName();

    if (name.size() > 5 && name.substr(name.size() - 5, 5) == "_help")
    {
        String wname = name.substr(0, name.size() - 5);
        std::pair<UTFString, UTFString> hText = helpTexts[wname];
        std::pair<Real, Real> minmax = minMaxs[wname];

        UTFString mTitle = hText.first;
        UTFString mTxt = hText.second;
        if (fabs(minmax.first - minmax.second) > 0.001f)
        {
            wchar_t param[512] = L"";
            UTFString fmt = _L("\nParameter range: %f to %f");
            swprintf(param, 512, fmt.asWStr_c_str(), minmax.first, minmax.second);
            mTxt = mTxt + UTFString(param);
        }
        MyGUI::IntPoint p = _sender->getAbsolutePosition();
        MyGUI::IntSize s = _sender->getSize();
        showHelp(mTitle, mTxt, p.left + s.width, p.top + s.height * 0.5f);
        return;
    }

    if (name == "select_current_ground")
    {
        updateControls(active_gm);
    }
    else if (_sender->getName() == "apply_changes")
    {
        applyChanges();
    }

    //LOG(" Friction GUI button pressed: " + _sender->getName());
}

void FrictionSettings::event_scroll_value(MyGUI::ScrollBar* _sender, size_t _value)
{
    String name = _sender->getName();
    if (name.size() > 7 && name.substr(name.size() - 7, 7) == "_scroll")
    {
        String wName = name.substr(0, name.size() - 7);
        MyGUI::EditPtr e = (MyGUI::EditPtr)win->findWidget(wName);
        std::pair<Real, Real> f = minMaxs[wName];
        MyGUI::ScrollBar* h = (MyGUI::ScrollBar *)_sender;
        Real rvalue = ((((float)_value) / (float)(h->getScrollRange())) * (f.second - f.first)) + f.first;
        if (e)
            e->setCaption(TOSTRING(rvalue));

        MyGUI::TextBox* edt = (MyGUI::TextBox *)win->findWidget(wName + "_edited");
        if (edt)
            edt->setCaption("!!");
    }
}

void FrictionSettings::applyChanges()
{
    MyGUI::TextBox* edt;

    MyGUI::EditPtr e = (MyGUI::EditPtr)win->findWidget("solid_adhension_velo");
    selected_gm->va = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_adhension_velo_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("solid_level");
    selected_gm->solid_ground_level = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_level_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("solid_static_friction");
    selected_gm->ms = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_static_friction_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("solid_dynamic_friction");
    selected_gm->mc = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_dynamic_friction_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("solid_hydrodynamic");
    selected_gm->t2 = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_hydrodynamic_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("solid_stribeck");
    selected_gm->vs = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_stribeck_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("solid_alpha");
    selected_gm->alpha = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_alpha_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("solid_strength");
    selected_gm->strength = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("solid_strength_edited");
    if (edt)
        edt->setCaption("");

    // fluid
    e = (MyGUI::EditPtr)win->findWidget("fluid_flowbeh");
    selected_gm->flow_behavior_index = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("fluid_flowbeh_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("fluid_flowcon");
    selected_gm->flow_consistency_index = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("fluid_flowcon_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("fluid_density");
    selected_gm->fluid_density = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("fluid_density_edited");
    if (edt)
        edt->setCaption("");

    e = (MyGUI::EditPtr)win->findWidget("fluid_drag_anisotropy");
    selected_gm->drag_anisotropy = StringConverter::parseReal(e->getCaption());
    edt = (MyGUI::TextBox *)win->findWidget("fluid_drag_anisotropy_edited");
    if (edt)
        edt->setCaption("");

}

void FrictionSettings::notifyWindowButtonPressed(MyGUI::WindowPtr _sender, const std::string& _name)
{
    if (_name == "close")
        this->SetVisible(false);
}

void FrictionSettings::notifyHelpWindowButtonPressed(MyGUI::WindowPtr _sender, const std::string& _name)
{
    if (_name == "close")
        msgwin->setVisibleSmooth(false);
}

void FrictionSettings::showHelp(UTFString title, UTFString msg, int x, int y)
{
    MyGUI::EditPtr e = (MyGUI::EditPtr)msgwin->findWidget("helptext");
    e->setCaption(convertToMyGUIString(msg));
    UTFString tmp = _L("Friction Help: ") + title;
    msgwin->setCaption(convertToMyGUIString(tmp));
    msgwin->setPosition(x + 20, y - 150);
    msgwin->setVisibleSmooth(true);
}

} // namespace GUI
} // namespace RoR
