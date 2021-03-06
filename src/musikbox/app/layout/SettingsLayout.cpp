//////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2007-2016 musikcube team
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the author nor the names of other contributors may
//      be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include <cursespp/Colors.h>
#include <cursespp/Screen.h>
#include <cursespp/SingleLineEntry.h>

#include <core/library/Indexer.h>
#include <core/library/LocalLibraryConstants.h>
#include <core/support/PreferenceKeys.h>

#include <app/query/SearchTrackListQuery.h>
#include <app/util/Hotkeys.h>
#include <app/util/PreferenceKeys.h>

#include "SettingsLayout.h"

using namespace musik;
using namespace musik::core::library::constants;
using namespace musik::core;
using namespace musik::box;
using namespace cursespp;
using namespace std::placeholders;

#define LABEL_HEIGHT 1
#define INPUT_HEIGHT 3
#define HOTKEY_INPUT_WIDTH 20

#define TOP(w) w->GetY()
#define BOTTOM(w) (w->GetY() + w->GetHeight())
#define LEFT(w) w->GetX()
#define RIGHT(w) (w->GetX() + w->GetWidth())

#define CREATE_CHECKBOX(checkbox, caption) \
    checkbox.reset(new cursespp::Checkbox()); \
    checkbox->SetText(caption); \
    checkbox->CheckChanged.connect(this, &SettingsLayout::OnCheckboxChanged);

using EntryPtr = IScrollAdapter::EntryPtr;

static bool showDotfiles = false;

SettingsLayout::SettingsLayout(musik::core::LibraryPtr library)
: LayoutBase()
, library(library)
, indexer(library->Indexer()) {
    this->prefs = Preferences::ForComponent(core::prefs::components::Settings);
    this->indexer->PathsUpdated.connect(this, &SettingsLayout::RefreshAddedPaths);
    this->InitializeWindows();
}

SettingsLayout::~SettingsLayout() {
}

void SettingsLayout::OnCheckboxChanged(cursespp::Checkbox* cb, bool checked) {
    if (cb == removeCheckbox.get()) {
        this->prefs->SetBool(core::prefs::keys::RemoveMissingFiles, checked);
        this->prefs->Save();
    }
    else if (cb == dotfileCheckbox.get()) {
        showDotfiles = !showDotfiles;
        this->browseAdapter.SetDotfilesVisible(showDotfiles);
        this->browseList->OnAdapterChanged();
    }
    else if (cb == focusShortcutsCheckbox.get()) {
        this->prefs->SetBool(box::prefs::keys::EscFocusesShortcuts, checked);
        this->prefs->Save();
    }
    else if (cb == customColorsCheckbox.get()) {
        this->prefs->SetBool(box::prefs::keys::DisableCustomColors, checked);
        this->prefs->Save();
    }
}

void SettingsLayout::Layout() {
    int x = this->GetX(), y = this->GetY();
    int cx = this->GetWidth(), cy = this->GetHeight();

    int startY = 0;
    int leftX = 0;
    int leftWidth = cx / 3; /* 1/3 width */
    int rightX = leftWidth;
    int rightWidth = cx - rightX; /* remainder (~2/3) */

    this->browseLabel->MoveAndResize(leftX + 1, startY, leftWidth - 1, LABEL_HEIGHT);
    this->addedPathsLabel->MoveAndResize(rightX + 1, startY, rightWidth - 1, LABEL_HEIGHT);

    int pathListsY = BOTTOM(this->browseLabel);
    int pathsHeight = (cy - pathListsY) / 2;

    this->browseList->MoveAndResize(leftX, pathListsY, leftWidth, pathsHeight);
    this->addedPathsList->MoveAndResize(rightX, pathListsY, rightWidth, pathsHeight);

    this->dotfileCheckbox->MoveAndResize(1, BOTTOM(this->browseList), cx - 1, LABEL_HEIGHT);
    this->removeCheckbox->MoveAndResize(1, BOTTOM(this->dotfileCheckbox), cx - 1, LABEL_HEIGHT);
    this->focusShortcutsCheckbox->MoveAndResize(1, BOTTOM(this->removeCheckbox), cx - 1, LABEL_HEIGHT);
    this->customColorsCheckbox->MoveAndResize(1, BOTTOM(this->focusShortcutsCheckbox), cx - 1, LABEL_HEIGHT);

    this->hotkeyLabel->MoveAndResize(
        1,
        BOTTOM(this->customColorsCheckbox) + 2,
        this->hotkeyLabel->Length(),
        LABEL_HEIGHT);

    this->hotkeyInput->MoveAndResize(
        RIGHT(this->hotkeyLabel),
        BOTTOM(this->customColorsCheckbox) + 1,
        HOTKEY_INPUT_WIDTH,
        INPUT_HEIGHT);
}

void SettingsLayout::RefreshAddedPaths() {
    this->addedPathsAdapter.Clear();

    std::vector<std::string> paths;
    this->indexer->GetPaths(paths);

    for (size_t i = 0; i < paths.size(); i++) {
        auto v = paths.at(i);
        auto e = EntryPtr(new SingleLineEntry(v));
        this->addedPathsAdapter.AddEntry(e);
    }

    this->addedPathsList->OnAdapterChanged();
}

int64 SettingsLayout::ListItemDecorator(
    ScrollableWindow* scrollable,
    size_t index,
    size_t line,
    IScrollAdapter::EntryPtr entry)
{
    if (scrollable == this->addedPathsList.get() ||
        scrollable == this->browseList.get())
    {
         ListWindow* lw = static_cast<ListWindow*>(scrollable);
         if (lw->GetSelectedIndex() == index) {
             return COLOR_PAIR(CURSESPP_HIGHLIGHTED_LIST_ITEM);
         }
    }
    return -1;
}

void SettingsLayout::InitializeWindows() {
    this->SetFrameVisible(false);

    this->browseLabel.reset(new TextLabel());
    this->browseLabel->SetText("browse (SPACE to add)", text::AlignLeft);

    this->addedPathsLabel.reset(new TextLabel());
    this->addedPathsLabel->SetText("indexed paths (BACKSPACE to remove)", text::AlignLeft);

    this->addedPathsList.reset(new cursespp::ListWindow(&this->addedPathsAdapter));
    this->browseList.reset(new cursespp::ListWindow(&this->browseAdapter));

    ScrollAdapterBase::ItemDecorator decorator =
        std::bind(
            &SettingsLayout::ListItemDecorator,
            this,
            std::placeholders::_1,
            std::placeholders::_2,
            std::placeholders::_3,
            std::placeholders::_4);

    this->addedPathsAdapter.SetItemDecorator(decorator);
    this->browseAdapter.SetItemDecorator(decorator);

    CREATE_CHECKBOX(this->dotfileCheckbox, "show dotfiles in directory browser");
    CREATE_CHECKBOX(this->removeCheckbox, "remove missing files from library");
    CREATE_CHECKBOX(this->focusShortcutsCheckbox, "esc key focuses shortcuts bar");
    CREATE_CHECKBOX(this->customColorsCheckbox, "disable custom colors (requires restart)");

    this->hotkeyLabel.reset(new TextLabel());
    this->hotkeyLabel->SetText("hotkey tester: ");
    this->hotkeyInput.reset(new TextInput(IInput::InputRaw));

    this->browseList->SetFocusOrder(0);
    this->addedPathsList->SetFocusOrder(1);
    this->dotfileCheckbox->SetFocusOrder(2);
    this->removeCheckbox->SetFocusOrder(3);
    this->focusShortcutsCheckbox->SetFocusOrder(4);
    this->customColorsCheckbox->SetFocusOrder(5);
    this->hotkeyInput->SetFocusOrder(6);

    this->AddWindow(this->browseLabel);
    this->AddWindow(this->addedPathsLabel);
    this->AddWindow(this->browseList);
    this->AddWindow(this->addedPathsList);
    this->AddWindow(this->dotfileCheckbox);
    this->AddWindow(this->removeCheckbox);
    this->AddWindow(this->focusShortcutsCheckbox);
    this->AddWindow(this->customColorsCheckbox);
    this->AddWindow(this->hotkeyLabel);
    this->AddWindow(this->hotkeyInput);
}

void SettingsLayout::SetShortcutsWindow(ShortcutsWindow* shortcuts) {
    if (shortcuts) {
        shortcuts->AddShortcut(Hotkeys::NavigateSettings, "settings");
        shortcuts->AddShortcut(Hotkeys::NavigateLibrary, "library");
        shortcuts->AddShortcut(Hotkeys::NavigateConsole, "console");
        shortcuts->AddShortcut("^D", "quit");
        shortcuts->SetActive(Hotkeys::NavigateSettings);
    }
}

void SettingsLayout::OnVisibilityChanged(bool visible) {
    LayoutBase::OnVisibilityChanged(visible);

    if (visible) {
        this->RefreshAddedPaths();
        this->LoadPreferences();
    }
}

void SettingsLayout::LoadPreferences() {
    this->removeCheckbox->SetChecked(this->prefs->GetBool(core::prefs::keys::RemoveMissingFiles, true));
    this->focusShortcutsCheckbox->SetChecked(this->prefs->GetBool(box::prefs::keys::EscFocusesShortcuts));
    this->customColorsCheckbox->SetChecked(this->prefs->GetBool(box::prefs::keys::DisableCustomColors));
}

void SettingsLayout::AddSelectedDirectory() {
    size_t index = this->browseList->GetSelectedIndex();
    std::string path = this->browseAdapter.GetFullPathAt(index);

    if (path.size()) {
        this->indexer->AddPath(path);
    }
}

void SettingsLayout::RemoveSelectedDirectory() {
    std::vector<std::string> paths;
    this->indexer->GetPaths(paths);
    size_t index = this->addedPathsList->GetSelectedIndex();
    this->indexer->RemovePath(paths.at(index));
}

void SettingsLayout::DrillIntoSelectedDirectory() {
    this->browseAdapter.Select(this->browseList->GetSelectedIndex());
    this->browseList->SetSelectedIndex(0);
    this->browseList->OnAdapterChanged();
}

bool SettingsLayout::KeyPress(const std::string& key) {
    if (key == "KEY_ENTER") {
        if (this->GetFocus() == this->browseList) {
            this->DrillIntoSelectedDirectory();
            return true;
        }
    }
    else if (key == " ") {
        if (this->GetFocus() == this->browseList) {
            this->AddSelectedDirectory();
            return true;
        }
    }
    else if (key == "KEY_BACKSPACE") { /* backspace */
        if (this->GetFocus() == this->addedPathsList) {
            this->RemoveSelectedDirectory();
            return true;
        }
    }

    return LayoutBase::KeyPress(key);
}
