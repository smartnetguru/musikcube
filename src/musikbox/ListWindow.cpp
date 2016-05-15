#include "stdafx.h"
#include "ListWindow.h"

typedef IScrollAdapter::ScrollPosition ScrollPos;

ListWindow::ListWindow(IWindow *parent) 
: ScrollableWindow(parent) {
    this->selectedIndex = 0;
}

ListWindow::~ListWindow() {

}

void ListWindow::ScrollToTop() {
    this->selectedIndex = 0;
    this->GetScrollAdapter().DrawPage(this->GetContent(), 0, &scrollPosition);
    this->Repaint();
}

void ListWindow::ScrollToBottom() {
    IScrollAdapter& adapter = this->GetScrollAdapter();
    this->selectedIndex = adapter.GetEntryCount() - 1;
    adapter.DrawPage(this->GetContent(), selectedIndex, &scrollPosition);
    this->Repaint();
}

void ListWindow::ScrollUp(int delta) {
    ScrollPos spos = this->GetScrollPosition();
    IScrollAdapter& adapter = this->GetScrollAdapter();

    size_t first = spos.firstVisibleEntryIndex;
    size_t last = first + spos.visibleEntryCount;
    int drawIndex = first;

    int minIndex = 0;
    int newIndex = this->selectedIndex - delta;
    newIndex = max(newIndex, minIndex);

    if (newIndex < (int) first + 1) {
        drawIndex = newIndex - 1;
    }

    selectedIndex = newIndex;

    drawIndex = max(0, drawIndex);
    adapter.DrawPage(this->GetContent(), drawIndex, &this->scrollPosition);

    this->Repaint();
}

void ListWindow::ScrollDown(int delta) {
    ScrollPos spos = this->GetScrollPosition();
    IScrollAdapter& adapter = this->GetScrollAdapter();

    size_t first = spos.firstVisibleEntryIndex;
    size_t last = first + spos.visibleEntryCount;
    size_t drawIndex = first;

    size_t maxIndex = adapter.GetEntryCount() - 1;
    size_t newIndex = this->selectedIndex + delta;
    newIndex = min(newIndex, maxIndex);

    if (newIndex >= last - 1) {
        drawIndex = drawIndex + delta;
    }

    selectedIndex = newIndex;

    adapter.DrawPage(this->GetContent(), drawIndex, &this->scrollPosition);

    this->Repaint();
}

void ListWindow::PageUp() {
    IScrollAdapter &adapter = this->GetScrollAdapter();
    ScrollPos spos = this->GetScrollPosition();
    int target = (int) this->GetPreviousPageEntryIndex();
    
    /* if the target position is zero, let it be so the user can see
    the top of the list. otherwise, scroll down by one to give indication
    there is more to see. */
    target = (target > 0) ? target + 1 : 0;
    this->selectedIndex = (target == 0) ? 0 : target + 1;

    adapter.DrawPage(this->GetContent(), target, &this->scrollPosition);
    this->Repaint();
}

void ListWindow::PageDown() {
    /* page down always makes the last item of this page, the first item
    of the next page, and selects the following item. */

    IScrollAdapter &adapter = this->GetScrollAdapter();
    ScrollPos spos = this->GetScrollPosition();

    size_t lastVisible = spos.firstVisibleEntryIndex + spos.visibleEntryCount - 1;
    this->selectedIndex = min(adapter.GetEntryCount() - 1, lastVisible + 1);

    adapter.DrawPage(this->GetContent(), lastVisible, &this->scrollPosition);
    this->Repaint();
}

size_t ListWindow::GetSelectedIndex() {
    return this->selectedIndex;
}

void ListWindow::OnAdapterChanged() {
    IScrollAdapter *adapter = &GetScrollAdapter();

    GetScrollAdapter().DrawPage(
        this->GetContent(),
        this->scrollPosition.firstVisibleEntryIndex,
        &this->scrollPosition);

    this->Repaint();
}

IScrollAdapter::ScrollPosition& ListWindow::GetScrollPosition() {
    return this->scrollPosition;
}