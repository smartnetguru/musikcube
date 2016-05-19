#pragma once

#include <stdafx.h>
#include "Window.h"
#include "IWindowGroup.h"
#include "WindowMessage.h"
#include "WindowMessageQueue.h"

static int NEXT_ID = 0;
static bool drawPending = false;

void Window::WriteToScreen() {
    if (drawPending) {
        drawPending = false;
        update_panels();
        doupdate();
    }
}

Window::Window(IWindow *parent) {
    this->frame = this->content = 0;
    this->framePanel = this->contentPanel = 0;
    this->parent = parent;
    this->height = 0;
    this->width = 0;
    this->x = 0;
    this->y = 0;
    this->contentColor = -1;
    this->frameColor = -1;
    this->drawFrame = true;
    this->isVisible = false;
    this->focusOrder = -1;
    this->id = NEXT_ID++;
}

Window::~Window() {
    WindowMessageQueue::Instance().Remove(this);
    this->Destroy();
}

int Window::GetId() const {
    return this->id;
}

void Window::ProcessMessage(IWindowMessage &message) {
   
}

bool Window::IsAcceptingMessages() {
    return true;
}

bool Window::IsVisible() {
    return this->isVisible;
}

void Window::BringToTop() {
    if (this->framePanel) {
        top_panel(this->framePanel);

        if (this->contentPanel != this->framePanel) {
            top_panel(this->contentPanel);
        }
    }
}

void Window::SendToBottom() {
    if (this->framePanel) {
        bottom_panel(this->contentPanel);

        if (this->contentPanel != this->framePanel) {
            bottom_panel(this->framePanel);
        }
    }
}

void Window::PostMessage(int messageType, int64 user1, int64 user2, int64 delay) {
    WindowMessageQueue::Instance().Post(
        WindowMessage::Create(
            this, 
            messageType, 
            user1, 
            user2),
        delay);
}

void Window::RemoveMessage(int messageType) {
    WindowMessageQueue::Instance().Remove(this, messageType);
}

void Window::SetParent(IWindow* parent) {
    if (this->parent != parent) {
        IWindowGroup* group = dynamic_cast<IWindowGroup*>(parent);

        if (group) {
            group->RemoveWindow(shared_from_this());
        }

        this->parent = parent;

        if (this->frame) {
            this->Hide();
            this->Show();
        }
    }
}

void Window::SetSize(int width, int height) {
    this->width = width;
    this->height = height;
}

void Window::SetPosition(int x, int y) {
    this->x = x;
    this->y = y;
}

int Window::GetWidth() const {
    return this->width;
}

int Window::GetHeight() const {
    return this->height;
}

int Window::GetContentHeight() const {
    if (!this->drawFrame) {
        return this->height;
    }

    return this->height ? this->height - 2 : 0;
}

int Window::GetContentWidth() const {
    if (!this->drawFrame) {
        return this->width;
    }

    return this->width ? this->width - 2 : 0;
}

int Window::GetX() const {
    return this->x;
}

int Window::GetY() const {
    return this->y;
}

void Window::SetContentColor(int64 color) {
    this->contentColor = color;

    if (this->contentColor != -1 && this->content) {
        wbkgd(this->frame, COLOR_PAIR(this->frameColor));

        if (this->content != this->frame) {
            wbkgd(this->content, COLOR_PAIR(this->contentColor));
        }

        this->Repaint();
    }
}

void Window::SetFrameColor(int64 color) {
    this->frameColor = color;

    if (this->drawFrame && this->frameColor != -1 && this->frame) {
        wbkgd(this->frame, COLOR_PAIR(this->frameColor));

        if (this->content != this->frame) {
            wbkgd(this->content, COLOR_PAIR(this->contentColor));
        }

        this->Repaint();
    }
}

WINDOW* Window::GetContent() const {
    return this->content;
}

WINDOW* Window::GetFrame() const {
    return this->frame;
}

int Window::GetFocusOrder() {
    return this->focusOrder;
}

void Window::SetFocusOrder(int order) {
    this->focusOrder = order;
}

void Window::Show() {
    if (this->framePanel) {
        show_panel(this->framePanel);

        if (this->framePanel != this->contentPanel) {
            show_panel(this->contentPanel);
        }

        this->isVisible = true;
        drawPending = true;
        return;
    }
    else {
        this->Create();
    }
}

void Window::Create() {
    /* if we have a parent, place the new window relative to the parent. */

    this->frame = (this->parent == NULL)
        ? newwin(
            this->height, 
            this->width, 
            this->y, 
            this->x)
        : newwin(
            this->height, 
            this->width, 
            this->parent->GetY() + this->y, 
            this->parent->GetX() + this->x);

    this->framePanel = new_panel(this->frame);

    /* if we were asked not to draw a frame, we'll set the frame equal to
    the content view, and use the content views colors*/

    if (!this->drawFrame) {
        this->content = this->frame;

        if (this->contentColor != -1) {
            wbkgd(this->frame, COLOR_PAIR(this->contentColor));
        }
    }

    /* otherwise we'll draw a box around the frame, and create a content
    sub-window inside */

    else {
        box(this->frame, 0, 0);

        this->content = newwin(
            this->height - 2,
            this->width - 2,
            this->GetY() + 1,
            this->GetX() + 1);

        this->contentPanel = new_panel(this->content);

        if (this->frameColor != -1) {
            wbkgd(this->frame, COLOR_PAIR(this->frameColor));
        }

        if (this->contentColor != -1) {
            wbkgd(this->content, COLOR_PAIR(this->contentColor));
        }
    }

    this->isVisible = true;
    this->Repaint();
}

void Window::Hide() {
    if (this->frame) {
        hide_panel(this->framePanel);

        if (this->content != this->frame) {
            hide_panel(this->contentPanel);
        }
    }

    this->isVisible = false;
}

void Window::Destroy() {
    this->Hide();

    if (this->frame) {
        del_panel(this->framePanel);
        delwin(this->frame);

        if (this->content != this->frame) {
            del_panel(this->contentPanel);
            delwin(this->content);
        }
    }

    this->framePanel = this->contentPanel = 0;
    this->content = this->frame = 0;
}

void Window::SetFrameVisible(bool enabled) {
    if (enabled != this->drawFrame) {
        this->drawFrame = enabled;

        if (this->frame || this->content) {
            this->Destroy();
            this->Show();
        }
    }
}

bool Window::IsFrameVisible() {
    return this->drawFrame;
}

IWindow* Window::GetParent() const {
    return this->parent;
}

void Window::Clear() {
    wclear(this->content);
}

void Window::Repaint() {
    if (this->isVisible) {
        if (this->frame) {
            drawPending = true;
        }
    }
}

void Window::Focus() {

}

void Window::Blur() {

}