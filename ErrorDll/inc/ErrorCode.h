#pragma once

#include "ErrorDll_define.h"

class ErrorCode_API ErrorCode
{
public:
    ErrorCode() = default;
    void crashHere(int *ptr);
};

