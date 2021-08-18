#pragma once
#ifdef ErrorCode_EXPORTS
#define ErrorCode_API __declspec(dllexport)
#else
#define ErrorCode_API __declspec(dllimport)
#endif


class ErrorCode_API ErrorCode
{
public:
    ErrorCode() = default;
    void crashHere(int *ptr);
};

