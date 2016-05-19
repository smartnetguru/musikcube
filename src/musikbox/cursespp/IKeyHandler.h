#pragma once

#include <stdafx.h>

class IKeyHandler {
    public:
        virtual ~IKeyHandler() = 0 { }
        virtual bool KeyPress(int64 ch) = 0;
};