/*
    This source file is part of Rigs of Rods
    Copyright 2005-2012 Pierre-Michel Ricordel
    Copyright 2007-2012 Thomas Fischer
    Copyright 2013-2017 Petr Ohlidal & contributors

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

/// This file was ported from MyGUI project (MIT licensed)
/// https://github.com/MyGUI/mygui
/// http://mygui.info/

/// @file
/// @author Albert Semenov
/// @date   08/2008

#pragma once

#include "Dialog.h"

#include <MyGUI.h>

namespace RoR
{
    namespace GUI
    {

        class OpenSaveFileDialog : public Dialog
        {
          public:
            OpenSaveFileDialog();

            void setDialogInfo(const MyGUI::UString &_caption, const MyGUI::UString &_button, bool _folderMode = false);

            void                  setCurrentFolder(const MyGUI::UString &_value);
            const MyGUI::UString &getCurrentFolder() const;

            void                  setFileName(const MyGUI::UString &_value);
            const MyGUI::UString &getFileName() const;

            const MyGUI::UString &getMode() const;
            void                  setMode(const MyGUI::UString &_value);

            typedef std::vector<MyGUI::UString> VectorUString;
            void                                setRecentFolders(const VectorUString &_listFolders);

            void                  setFileMask(const MyGUI::UString &_value);
            const MyGUI::UString &getFileMask() const;

          protected:
            virtual void onDoModal();
            virtual void onEndModal();

          private:
            void notifyWindowButtonPressed(MyGUI::Window *_sender, const std::string &_name);
            void notifyDirectoryComboAccept(MyGUI::ComboBox *_sender, size_t _index);
            void notifyDirectoryComboChangePosition(MyGUI::ComboBox *_sender, size_t _index);
            void notifyListChangePosition(MyGUI::ListBox *_sender, size_t _index);
            void notifyListSelectAccept(MyGUI::ListBox *_sender, size_t _index);
            void notifyEditSelectAccept(MyGUI::EditBox *_sender);
            void notifyMouseButtonClick(MyGUI::Widget *_sender);
            void notifyUpButtonClick(MyGUI::Widget *_sender);

            void update();
            void accept();

            void upFolder();

          private:
            MyGUI::Window *  mWindow;
            MyGUI::ListBox * mListFiles;
            MyGUI::EditBox * mEditFileName;
            MyGUI::Button *  mButtonUp;
            MyGUI::ComboBox *mCurrentFolderField;
            MyGUI::Button *  mButtonOpenSave;

            MyGUI::UString mCurrentFolder;
            MyGUI::UString mFileName;
            MyGUI::UString mFileMask;

            MyGUI::UString mMode;
            bool           mFolderMode;
        };

    } // namespace GUI
} // namespace RoR
