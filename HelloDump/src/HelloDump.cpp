// HelloDump.cpp : 此檔案包含 'main' 函式。程式會於該處開始執行及結束執行。
//

// VC 中使用#include命令包含標頭檔案所搜尋的路徑：
// 1. 系統路徑 2. 當前路徑 3. 附加路徑

// 1. #include <file.h>
//      just 搜尋 "系統路徑"。
// 2. #include "file.h"
//      first 搜尋 "附加路徑"，second 搜尋 "系統路徑"，final 搜索 "當前路徑"。
// 3. #include "directory\file.h"
//      just 搜尋 "指定的路徑"。

#include <iostream>
#include <windows.h>

#include "..\..\ErrorDll\inc\ErrorCode.h"
#pragma comment(lib,"ErrorDll.lib")

#include <DbgHelp.h>
#pragma comment(lib,"Dbghelp.lib")

static long  __stdcall callbackUnhandledException(LPEXCEPTION_POINTERS pexcp);
int main()
{
    std::cout << "Hello Dump!\n";

    // 調用該函數後，如果一個未被調試的進程發生異常，並且該異常進入未處理異常過濾器，
    // 該過濾器將調用lpTopLevelExceptionFilter參數指定的異常過濾器函數 (callback)。

    // return:      the address of the previous exception filter established with the function.
    // return NULL: there is no current top-level exception handler.
    ::SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)callbackUnhandledException);

    ErrorCode goCrash;
    goCrash.crashHere(nullptr);
    return 0;
}

// EXCEPTION_EXECUTE_HANDLER    ==  1 表示我已經處理了異常，可以結束。
// EXCEPTION_CONTINUE_SEARCH    ==  0 表示我不處理，其他人來做吧，於是windows 調用一個default 處理程序顯示一個錯誤，並結束。
// EXCEPTION_CONTINUE_EXECUTION == -1 表是錯誤已經被修正，請從異常發生處繼續執行。
long  __stdcall callbackUnhandledException(LPEXCEPTION_POINTERS  pexcp)
{
    // 建立 Dump 檔案
    // https://docs.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
    // HANDLE CreateFile(
    //     LPCTSTR lpFileName,                          // 指向文件名
    //     DWORD dwDesiredAccess,                       // 訪問模式
    //     DWORD dwShareMode,                           // 共享模式 (0 / 1)。
    //     LPSECURITY_ATTRIBUTES lpSecurityAttributes,  // 指向安全屬性
    //     DWORD dwCreationDisposition,                 // 如何創建
    //     DWORD dwFlagsAndAttributes,                  // 文件屬性
    //     HANDLE hTemplateFile                         // 用於複製文件句柄
    // );

    HANDLE hDumpFile = ::CreateFile(L"HelloDump.DMP",
                                    GENERIC_WRITE,
                                    0,
                                    NULL,
                                    CREATE_ALWAYS,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);


    if (hDumpFile != INVALID_HANDLE_VALUE)
    {
        //Dump 資訊
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pexcp;
        dumpInfo.ThreadId = ::GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        // 寫入Dump檔案內容
        // https://docs.microsoft.com/en-us/windows/win32/api/minidumpapiset/nf-minidumpapiset-minidumpwritedump
        ::MiniDumpWriteDump(::GetCurrentProcess(),
                            ::GetCurrentProcessId(),
                            hDumpFile,
                            MiniDumpNormal,
                            &dumpInfo,
                            NULL,
                            NULL);
    }
    return 0;
}

// PE檔案中包含了對應的pdb檔案的校驗碼和路徑，二者必須和pdb檔案保持一致，即：
// PE檔案和pdb檔案必須同時生成，pdb檔案必須在PE檔案中記錄的路徑下，而pdb檔案中則記錄了原始碼的校驗碼和路徑。
