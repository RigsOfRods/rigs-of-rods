/*
    This source file is part of Rigs of Rods
    Copyright 2013-2016 Petr Ohlidal & contributors

    For more information, see http://www.rigsofrods.com/

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

#pragma once

/** 
    @file   
    @author Petr Ohlidal
    @date   01/2015
*/

#include "ForwardDeclarations.h"
#include "BitFlags.h"
#include "GuiPanelBase.h"
#include "RigEditor_ForwardDeclarations.h"

#include <MyGUI_Prerequest.h> // Forward declarations
#include <MyGUI_Colour.h>
#include <OgreString.h>

namespace RoR
{

namespace RigEditor
{

class RigElementGuiPanelBase: public GuiPanelBase
{

public:

    // ------------------------------------------------------------------------
    struct FlagCheckboxUserData
    {
        FlagCheckboxUserData(unsigned int* flags_ptr, unsigned int uniformity_flag, unsigned int value_flag, const char* tooltip, MyGUI::TextBox* textbox):
            uniformity_flag(uniformity_flag),
            value_flag(value_flag),
            tooltip_text(tooltip),
            tooltip_textbox(textbox),
            data_flags_ptr(flags_ptr)
        {}

        unsigned int    uniformity_flag;
        unsigned int    value_flag;
        unsigned int*   data_flags_ptr;
        const char *    tooltip_text;
        MyGUI::TextBox* tooltip_textbox;
    };
    // ------------------------------------------------------------------------
    struct GenericFieldSpec
    {
        GenericFieldSpec(
                MyGUI::TextBox* label, MyGUI::Widget* field_widget, 
                unsigned int* source_flags_ptr, unsigned int uniformity_flag, 
                void* source_ptr, unsigned int source_type_flag, bool check_uniformity = true
            ):
            label(label),
            field_widget(field_widget),
            m_flags(source_type_flag),
            m_source_ptr(source_ptr),
            m_source_flags_ptr(source_flags_ptr),
            m_use_uniformity(check_uniformity),
            m_uniformity_flag(uniformity_flag)
        {}

        GenericFieldSpec():
            label(nullptr),
            field_widget(nullptr),
            m_flags(0),
            m_source_ptr(nullptr),
            m_uniformity_flag(0),
            m_use_uniformity(true)
        {}

        BITMASK_PROPERTY(m_flags, 1, SOURCE_DATATYPE_FLOAT,       IsSourceFloat,  SetSourceIsFloat)
        BITMASK_PROPERTY(m_flags, 2, SOURCE_DATATYPE_INT,         IsSourceInt,    SetSourceIsInt)
        BITMASK_PROPERTY(m_flags, 3, SOURCE_DATATYPE_STRING,      IsSourceString, SetSourceIsString)
        BITMASK_PROPERTY(m_flags, 4, SOURCE_DATATYPE_BOOL,        IsSourceBool,   SetSourceIsBool)
        BITMASK_PROPERTY(m_flags, 5, SOURCE_DATATYPE_OBJECT_NODE, IsSourceNode,   SetSourceIsNode)

        inline float*         GetSourceFloat()  { return (IsSourceFloat()  ? reinterpret_cast<float*>       (m_source_ptr) : nullptr); }
        inline int*           GetSourceInt()    { return (IsSourceInt()    ? reinterpret_cast<int*>         (m_source_ptr) : nullptr); }
        inline Ogre::String*  GetSourceString() { return (IsSourceString() ? reinterpret_cast<Ogre::String*>(m_source_ptr) : nullptr); }
        inline bool*          GetSourceBool()   { return (IsSourceBool()   ? reinterpret_cast<bool*>        (m_source_ptr) : nullptr); }
        inline void*          GetSourceRawPtr() const { return m_source_ptr; }

        inline void SetSourceIsUniform()
        {
            if (m_use_uniformity)
                BITMASK_SET_1(*m_source_flags_ptr, m_uniformity_flag);
        }
        inline void SetSourceNotUniform()
        {
            if (m_use_uniformity)
                BITMASK_SET_0(*m_source_flags_ptr, m_uniformity_flag);
        }
        inline bool IsSourceUniform() const 
        { 
            return m_use_uniformity ? BITMASK_IS_1(*m_source_flags_ptr,  m_uniformity_flag) : true;
        }
        void SetWidgetsVisible(bool visible);

        MyGUI::TextBox*  label;
        MyGUI::Widget*   field_widget;
        
    private:
        void*            m_source_ptr;
        unsigned int*    m_source_flags_ptr;
        bool             m_use_uniformity;
        unsigned int     m_uniformity_flag;
        unsigned int     m_flags;
    };
    // ------------------------------------------------------------------------
    struct EditboxFieldSpec: public GenericFieldSpec
    {
        EditboxFieldSpec(
                MyGUI::TextBox* label, 
                MyGUI::EditBox* editbox, 
                unsigned int* source_flags_ptr, 
                unsigned int uniformity_flag, 
                void* source_ptr, 
                unsigned int source_type_flag, 
                bool use_uniformity);

        EditboxFieldSpec():
            GenericFieldSpec(),
            editbox(nullptr)
        {}

        MyGUI::EditBox*  editbox;
    };
    // ------------------------------------------------------------------------

    RigElementGuiPanelBase(
        RigEditor::IMain* rig_editor_interface, 
        RigEditor::Config* config, 
        MyGUI::Window* panel_widget,
        MyGUI::Button* text_color_source
    );

    inline void SetDefaultTextColor(MyGUI::Colour const & col) { m_text_color_default = col; }

    BITMASK_PROPERTY( m_flags,  1,  IMMEDIATE_RIG_UPDATE_NEEDED, IsImmediateRigUpdateNeeded, SetIsImmediateRigUpdateNeeded);

protected:

    // Notification calback
    virtual void OnFieldValueChanged(GenericFieldSpec* field) {}

    // Flag checkbox
    void SetupFlagCheckbox(
        MyGUI::Button* checkbox, 
        unsigned int* data_flags_ptr, 
        unsigned int flag_uniform, 
        unsigned int flag_value, 
        MyGUI::TextBox* tooltip_label, 
        const char* tooltip_text
    );
    void CallbackClick_FlagCheckboxUpdateValue(MyGUI::Widget* sender);
    void CallbackGotMouseFocus_FlagCheckboxShowTooltip(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget);
    void CallbackLostMouseFocus_FlagCheckboxClearTooltip(MyGUI::Widget* old_widget, MyGUI::Widget* new_widget);
    void UpdateFlagCheckbox(MyGUI::Button* checkbox, bool selected, bool mixed);

    // Editbox form field

    void SetupEditboxField(
        EditboxFieldSpec* spec, 
        MyGUI::TextBox* label, 
        MyGUI::EditBox* editbox, 
        unsigned int* source_flags_ptr, 
        unsigned int uniformity_flag, 
        void* source_ptr, 
        unsigned int source_type_flag,
        bool use_uniformity = true
    );
    void CallbackKeyPress_EnterCommitsEscapeRestores(MyGUI::Widget* widget, MyGUI::KeyCode key_code, MyGUI::Char key_value);
    void CallbackKeyFocusGained_RestorePreviousFieldValue(MyGUI::Widget* new_widget, MyGUI::Widget* old_widget);
    void EditboxRestoreValue(EditboxFieldSpec* spec);
    void EditboxCommitValue(EditboxFieldSpec* spec);

    // Generic form field

    void SetupGenericField(
        GenericFieldSpec* spec, 
        MyGUI::TextBox* label, 
        MyGUI::Widget* field_widget, 
        unsigned int* source_flags_ptr, 
        unsigned int uniformity_flag, 
        void* source_ptr, 
        unsigned int source_type_flag
    );

    // Combobox field

    void ComboboxRestoreValue(GenericFieldSpec* spec);
    void ComboboxCommitValue(GenericFieldSpec* spec);

    // GUI panel utility
    void AlignToScreen(RigEditor::GuiPanelPositionData* position_data);

    RigEditor::IMain* m_rig_editor_interface;
    unsigned int m_flags;

    MyGUI::Colour m_text_color_mixvalues;
    MyGUI::Colour m_text_color_default;
    MyGUI::Colour m_text_color_tooltip;

};

} // namespace RigEditor

} // namespace RoR
