#include "pch.h"
#include "ErrorCode.h"

void ErrorCode::crashHere(int* ptr) {
    ptr[5] = 1;
};
