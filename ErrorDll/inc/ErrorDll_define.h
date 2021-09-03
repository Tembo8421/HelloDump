#pragma once
#ifdef ERRORDLL_EXPORTS
#define ErrorCode_API __declspec(dllexport)
#else
#define ErrorCode_API __declspec(dllimport)
#endif