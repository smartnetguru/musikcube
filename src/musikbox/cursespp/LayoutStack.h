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

#pragma once

#include "ILayout.h"
#include "ILayoutStack.h"
#include "LayoutBase.h"

#include <memory>

namespace cursespp {
    class LayoutStack : public LayoutBase, public ILayoutStack {
        public:
            LayoutStack();
            virtual ~LayoutStack();

            /* ILayout */
            virtual IWindowPtr FocusNext();
            virtual IWindowPtr FocusPrev();
            virtual IWindowPtr GetFocus();
            virtual ILayoutStack* GetLayoutStack();
            virtual void SetLayoutStack(ILayoutStack* stack);
            virtual void Layout() { }

            /* IOrderable */
            virtual void BringToTop();
            virtual void SendToBottom();

            /* IDisplayable */
            virtual void Show();
            virtual void Hide();

            /* IKeyHandler */
            virtual bool KeyPress(const std::string& key);

            /* IWindowGroup */
            virtual bool AddWindow(IWindowPtr window);
            virtual bool RemoveWindow(IWindowPtr window);
            virtual size_t GetWindowCount();
            virtual IWindowPtr GetWindowAt(size_t position);

            /* ILayoutStack */
            virtual bool Push(ILayoutPtr layout);
            virtual bool Pop(ILayoutPtr layout);
            virtual bool BringToTop(ILayoutPtr layout);
            virtual bool SendToBottom(ILayoutPtr layout);
            virtual ILayoutPtr Top();

        private:
            std::deque<ILayoutPtr> layouts;
            ILayoutStack* stack;
            bool visible;
    };
}
