#include "stdafx.h"
#include "Profiler.h"
#include <cstdlib>
#include <tchar.h>

static std::filesystem::path ProgramPath;

void Profiler::OpenProfiler()
{
    STARTUPINFO info = { sizeof(info) };
    PROCESS_INFORMATION processInfo;
    TCHAR cmd[] = _T("profiler/Tracy.exe -a 127.0.0.1");

    if (CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo)) 
    {
        CloseHandle(processInfo.hProcess); // Cleanup since you don't need this
        CloseHandle(processInfo.hThread); // Cleanup since you don't need this
    }
}
