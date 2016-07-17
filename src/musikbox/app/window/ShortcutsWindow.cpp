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
#include "ShortcutsWindow.h"

#include <cursespp/Colors.h>
#include <cursespp/Text.h>

#include <boost/algorithm/string.hpp>

using namespace cursespp;
using namespace musik::box;

ShortcutsWindow::ShortcutsWindow()
: Window(nullptr) {
    this->SetFrameVisible(false);
    this->UpdateContentColor();
}

ShortcutsWindow::~ShortcutsWindow() {
}

void ShortcutsWindow::AddShortcut(
    const std::string& key,
    const std::string& description,
    int64 attrs)
{
    this->entries.push_back(
        std::shared_ptr<Entry>(new Entry(key, description, attrs)));

    this->Repaint();
}

void ShortcutsWindow::AddShortcut(
    Hotkeys::Id id,
    const std::string& description,
    int64 attrs)
{
    this->AddShortcut(Hotkeys::Get(id), description, attrs);
}

void ShortcutsWindow::SetActive(const std::string& key) {
    this->activeKey = key;
    this->Repaint();
}

void ShortcutsWindow::OnFocusChanged(bool focused) {
    this->UpdateContentColor();
    this->Repaint();
}

void ShortcutsWindow::SetActive(Hotkeys::Id id) {
    this->SetActive(Hotkeys::Get(id));
}

void ShortcutsWindow::UpdateContentColor() {
    this->SetContentColor(this->IsFocused()
        ? CURSESPP_BUTTON_NEGATIVE
        : CURSESPP_SELECTED_LIST_ITEM);
}

void ShortcutsWindow::Repaint() {
    Window::Repaint();


    this->Clear();

    int64 normalAttrs = COLOR_PAIR(CURSESPP_BUTTON_NORMAL);
    int64 activeAttrs = COLOR_PAIR(CURSESPP_BUTTON_HIGHLIGHTED);

    WINDOW* c = this->GetContent();

    size_t remaining = this->GetContentWidth();
    for (size_t i = 0; i < this->entries.size() && remaining > 0; i++) {
        auto e = this->entries[i];

        int64 keyAttrs = (e->attrs == -1) ? normalAttrs : COLOR_PAIR(e->attrs);
        keyAttrs = (e->key == this->activeKey) ? activeAttrs : keyAttrs;

        wprintw(c, " ");
        --remaining;

        if (remaining == 0) {
            continue;
        }

        std::string key = " " + e->key + " ";
        std::string value = " " + e->description + " ";

        size_t len = u8cols(key);
        if (len > remaining) {
            key = text::Ellipsize(key, remaining);
            len = remaining;
        }

        wattron(c, keyAttrs);
        wprintw(c, key.c_str());
        wattroff(c, keyAttrs);

        remaining -= len;

        if (remaining == 0) {
            continue;
        }

        len = u8cols(value);
        if (len > remaining) {
            value = text::Ellipsize(value, remaining);
            len = remaining;
        }

        wprintw(c, value.c_str());

        remaining -= len;
    }
}
