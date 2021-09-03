// HelloDump.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

#include <iostream>
#include <windows.h>
#include <strsafe.h>
#include <DbgHelp.h>
#pragma comment(lib,"Dbghelp.lib")

#include "crash_function.h"
#include <ctime>

#include "..\..\ErrorDll\inc\ErrorCode.h"
#pragma comment(lib,"ErrorDll.lib")

static HANDLE GenerateDump(EXCEPTION_POINTERS* pExceptionPointers);

static LONG WINAPI OurSetUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

//---------------------------------------------------------------------
// main
//---------------------------------------------------------------------
int main()
{
    std::cout << "Hello Dump!\n";
    // 設定崩潰Callback函式
    // 調用該函數後，如果一個未被調試的進程發生異常，並且該異常進入未處理異常過濾器，
    // 該過濾器將調用lpTopLevelExceptionFilter參數指定的異常過濾器函數 (callback)。
    // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter

    ::SetUnhandledExceptionFilter(OurSetUnhandledExceptionFilter);

    // 在某些情況下，C/C++ 運行時庫(CRT)將呼叫::SetUnhandledExceptionFilter(NULL)刪除任何自定義崩潰處理程序，
    // 並且永遠不會調用我們的崩潰處理程序。

    srand(static_cast<unsigned int>(std::time(0)));
    int option = rand() % 6;

    switch (option)
    {
    case 0:
        crash_function::MemoryAccessCrash();
        break;
    case 1:
        crash_function::OutOfBoundsVectorCrash();
        break;
    case 2: // CRT delete OurSetUnhandledExceptionFilter
        crash_function::AbortCrash();
        break;
    case 3: // CRT delete OurSetUnhandledExceptionFilter
        crash_function::VirtualFunctionCallCrash();
        break;
    case 4:
        std::cout << "MemoryAccessCrash in Error.dll!" << std::endl;
        ErrorCode goCrash;
        goCrash.crashHere(nullptr);
        break;
    case 5:
        std::cout << "RaiseException: 0xc0000374" << std::endl;
        // https://blog.csdn.net/tz_zs/article/details/77427842
        RaiseException(0xc0000374, 0, 0, NULL);
        break;
    }
    return 0;
}
//---------------------------------------------------------------------
// UnhandledExceptionFilter callback function
//---------------------------------------------------------------------

static LONG WINAPI OurSetUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    std::cout << "Gotcha !" << std::endl;
    auto result = MessageBoxA(NULL, "An exception has been caught. Do you wish to continue the execution of the program?", "Exception handling", MB_ICONQUESTION | MB_YESNO);
    if (result == IDYES)
        return EXCEPTION_CONTINUE_EXECUTION;
    else {
        if (GenerateDump(ExceptionInfo) == INVALID_HANDLE_VALUE)
            exit(-1);
        return EXCEPTION_EXECUTE_HANDLER;
    }
    // EXCEPTION_EXECUTE_HANDLER    ==  1 表示我已經處理了異常，可以結束，系統的Event Viewer 中不會紀錄異常。
    // EXCEPTION_CONTINUE_SEARCH    ==  0 表示我不處理，其他人來做吧，於是windows 調用一個default 處理程序，依賴於SetErrorMode標記。
    // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-seterrormode
    // EXCEPTION_CONTINUE_EXECUTION == -1 表是錯誤已經被修正，請從異常發生處繼續執行。
    return EXCEPTION_CONTINUE_SEARCH;
}

// dummy
static LONG WINAPI RedirectedSetUnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo)
{
    // When the CRT calls SetUnhandledExceptionFilter with NULL parameter
    // our handler will not get removed.
    std::cout << "Redirected !" << std::endl;
    return EXCEPTION_CONTINUE_SEARCH;
}

//---------------------------------------------------------------------
// Generate mini dump file
//---------------------------------------------------------------------
static BOOL IsDataSectionNeeded(const WCHAR* pModuleName)
{
    if (pModuleName == NULL)
    {
        return FALSE;
    }
    WCHAR szFileName[_MAX_FNAME] = L"";
    _wsplitpath_s(pModuleName, NULL, NULL, NULL, NULL, szFileName, _MAX_FNAME, NULL, NULL);
    if (wcscmp(szFileName, L"ntdll") == 0)
        return TRUE;
    return FALSE;
}

static BOOL CALLBACK MiniDumpCallback(PVOID                            pParam,
                                      const PMINIDUMP_CALLBACK_INPUT   pInput,
                                      PMINIDUMP_CALLBACK_OUTPUT        pOutput)
{
    // https://www.cnblogs.com/yilang/p/11897664.html
    if (pInput == 0 || pOutput == 0)
        return FALSE;
    switch (pInput->CallbackType)
    {
    case ModuleCallback:
        if (pOutput->ModuleWriteFlags & ModuleWriteDataSeg)
            if (!IsDataSectionNeeded(pInput->Module.FullPath))
                pOutput->ModuleWriteFlags &= (~ModuleWriteDataSeg);
    case IncludeModuleCallback:
    case IncludeThreadCallback:
    case ThreadCallback:
    case ThreadExCallback:
        return TRUE;
    default:;
    }
    return FALSE;
}

// https://docs.microsoft.com/zh-tw/windows/win32/dxtecharts/crash-dump-analysis
static HANDLE GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    std::cerr << "Error code: " << pExceptionPointers->ExceptionRecord->ExceptionCode << std::endl;

    WCHAR szPath[MAX_PATH];
    WCHAR szFileName[MAX_PATH];
    WCHAR* szAppName = (WCHAR*)L"HelloDump";
    WCHAR* szVersion = (WCHAR*)L"v1.0";
    DWORD dwBufferSize = MAX_PATH;

    // current time
    SYSTEMTIME stLocalTime;
    GetLocalTime(&stLocalTime);

    // %Temp%\HelloDump
    GetTempPath(dwBufferSize, szPath);
    StringCchPrintf(szFileName, MAX_PATH, L"%s%s", szPath, szAppName);
    CreateDirectory(szFileName, NULL);

    // v1.0-yyyymmdd-hhmmss-CurrentProcessId-CurrentThreadId.dmp
    StringCchPrintf(szFileName, MAX_PATH, L"%s%s\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
        szPath, szAppName, szVersion,
        stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
        stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
        GetCurrentProcessId(), GetCurrentThreadId());

    MessageBox(NULL, szFileName, NULL, MB_OK);
    // 建立 Dump 檔案
    // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
    HANDLE hDumpFile;
    hDumpFile = CreateFile(szFileName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        0,
        CREATE_ALWAYS,
        0,
        0);
    if (hDumpFile && hDumpFile != INVALID_HANDLE_VALUE)
    {
        //Dump 資訊
        // https://docs.microsoft.com/en-us/windows/win32/api/minidumpapiset/ns-minidumpapiset-minidump_exception_information
        BOOL bMiniDumpSuccessful;
        MINIDUMP_EXCEPTION_INFORMATION ExpParam;
        ExpParam.ThreadId = GetCurrentThreadId();
        ExpParam.ExceptionPointers = pExceptionPointers;
        ExpParam.ClientPointers = FALSE;

        // 可利用 MiniDumpCallback 函數來決定是否要將某些數據包含到 mini dump 中
        MINIDUMP_CALLBACK_INFORMATION mci;
        mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE)MiniDumpCallback;
        mci.CallbackParam = NULL;

        // 寫入Dump檔案內容
        // https://docs.microsoft.com/en-us/windows/win32/api/minidumpapiset/nf-minidumpapiset-minidumpwritedump
        bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(),
            GetCurrentProcessId(),
            hDumpFile,
            MiniDumpNormal,
            &ExpParam,
            NULL,
            &mci);

        if (!bMiniDumpSuccessful)
            std::cout <<"MiniDumpWriteDump failed. Error: "<< GetLastError() << std::endl;
        else
            std::cout << "Minidump created at the path:" << szFileName << std::endl;
    }

    return hDumpFile;
}


// reference: 
// https://www.codeproject.com/articles/154686/setunhandledexceptionfilter-and-the-c-c-runtime-li
// https://www.shuzhiduo.com/A/lk5avvDld1/
// https://www.itread01.com/content/1549809757.html
