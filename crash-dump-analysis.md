# Crash dump analysis
---
要在產品發佈前找到所有的Bug是一件幾乎不可能的事！微軟於 Platform SDK 中提供了可以幫助開發人員收集用戶端發現的異常信息的功能。該[**MiniDumpWriteDump**](https://docs.microsoft.com/zh-tw/windows/win32/api/minidumpapiset/nf-minidumpapiset-minidumpwritedump)函式會將必要的損毀傾印資訊(crash dump information)寫入檔案，而不會儲存整個進程空間。 此損毀傾印資訊檔案稱為小型傾印(minidump)。 以下提供如何產生和使用小型傾印檔的相關資訊。
<br/>
## What is dump file?
---
Dump file 是進程記憶體快照，紀錄程式當前的執行狀態。
最常應用於程式崩潰時產生當前進程狀態的紀錄檔，以便事後分析。
## 分類
1. Kernel-Mode Dump Files，主要用於驅動程式。
2. User-Mode Dump Files，主要是應用程式及服務程式。
	- Full Dumps: 完整的記憶體快照，檔案比較大。
	- Minidumps: 依照生成選項，生成包含不同資訊的Dump檔案。
<br/>
## How to dump?
---
### 1. 手動
#### - 工作管理員
&emsp;&emsp;滑鼠右鍵 >> 建立傾印檔
#### - Microsoft Visual Studio Team System 中的產品
&emsp;&emsp;Debug >> Save Dump As

### 2. 寫入程式碼
- [**SetUnhandledExceptionFilter function**](https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter)
- [**MiniDumpWriteDump**](https://docs.microsoft.com/zh-tw/windows/win32/api/minidumpapiset/nf-minidumpapiset-minidumpwritedump)
- 這些 API 函式是 DBGHELP 庫的一部分，而這個庫不是線程安全的，所以任何使用**MiniDumpWriteDump 的**程序都應該在嘗試調用**MiniDumpWriteDump**之前同步所有線程。
- Sample C++ code from [**here**](https://docs.microsoft.com/zh-tw/windows/win32/dxtecharts/crash-dump-analysis)
```C++=
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>

int GenerateDump(EXCEPTION_POINTERS* pExceptionPointers)
{
    BOOL bMiniDumpSuccessful;
    WCHAR szPath[MAX_PATH]; 
    WCHAR szFileName[MAX_PATH]; 
    WCHAR* szAppName = L"AppName";
    WCHAR* szVersion = L"v1.0";
    DWORD dwBufferSize = MAX_PATH;
    HANDLE hDumpFile;
    SYSTEMTIME stLocalTime;
    MINIDUMP_EXCEPTION_INFORMATION ExpParam;

    GetLocalTime( &stLocalTime );
    GetTempPath( dwBufferSize, szPath );

    StringCchPrintf( szFileName, MAX_PATH, L"%s%s", szPath, szAppName );
    CreateDirectory( szFileName, NULL );

    StringCchPrintf( szFileName, MAX_PATH, L"%s%s\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp", 
               szPath, szAppName, szVersion, 
               stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay, 
               stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond, 
               GetCurrentProcessId(), GetCurrentThreadId());
    hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE, 
                FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

    ExpParam.ThreadId = GetCurrentThreadId();
    ExpParam.ExceptionPointers = pExceptionPointers;
    ExpParam.ClientPointers = TRUE;

    bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), 
                    hDumpFile, MiniDumpWithDataSegs, &ExpParam, NULL, NULL);

    return EXCEPTION_EXECUTE_HANDLER;
}

void SomeFunction()
{
    __try
    {
        int *pBadPtr = NULL;
        *pBadPtr = 0;
    }
    __except(GenerateDump(GetExceptionInformation()))
    {
    }
}
````

- Google Breakpad: 開源的跨平台的崩溃转储和分析套件
https://github.com/google/breakpad/blob/master/docs/getting_started_with_breakpad.md
https://github.com/google/breakpad/tree/master/docs
  * 默認情況下，軟體崩潰時 breakpad 會生成 minidump 文件，在不同平台上的實現機制不同， 而它解決了跨平台的問題。
  * 目前使用Breakpad的有谷歌瀏覽器，火狐，卡米諾，谷歌地球，等專案。  
  * License是BSD的，可在商業軟體中使用！  
  * 如果以進程外捕獲Dump的方式，會需要實現 Client /Server 結構来處理崩潰進程抓取Dump 的請求。

### 3. 修改登錄檔
設定 MyApp.exe 崩壞時自動生成 mini dump
1. win + r 輸入 regedit
2.  到 `"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting"` 下建立一項 LocalDumps
	* LocalDumps 是全域性的，如果想針對指定程序設定，如MyApp.exe，則在/LocalDumps下新建子項 如 MyApp.exe
3. 於建立項目內加入以下配置：

	| Value | Type | default | 描述 |
	| :-----| :----: | :----: | :---- |
	| DumpFolder | REG_EXPAND_SZ | "D:\MyAppDump" | 檔案儲存路徑|
	| DumpCount | REG_DWORD | 10 |dump檔案的最大數目 |
	| DumpType | REG_DWORD | 1 |0:Custom dump; 1:Mini dump; 2:Full dump|
																			 
4. 可以寫成.bat：
* 設定登錄檔：
>@echo off  
echo Set MyApp.exe Dump...
reg add `"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MyApp.exe"`  
reg add `"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MyApp.exe"` /v DumpFolder /t REG_EXPAND_SZ /d "D:\MyAppDump" /f  
reg add `"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MyApp.exe"` /v DumpType /t REG_DWORD /d 1 /f  
reg add `"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps\MyApp.exe"` /v DumpCount /t REG_DWORD /d 10 /f  
echo Dump already set
pause  
@echo on 

* 取消設定：
>@echo off  
echo cancel MyApp.exe Dump...  
reg delete `"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps"` /f  
echo Dump already cancel
pause  
@echo on 
