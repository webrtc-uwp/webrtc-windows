/*
* This header is injected using /FI cl.exe flag for winrt project.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND OTHER CONTRIBUTORS ``AS IS''
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR OTHER CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <windows.h>

#define RtlGenRandom(xRandomBuffer,xRandomBufferLength) \
  winrtRtlGenRandom(xRandomBuffer,xRandomBufferLength)

#define MessageBoxA(xWnd,xText,xCaption,xType) \
  winrtMessageBoxA(xWnd,xText,xCaption,xType)

#define MessageBoxW(xWnd,xText,xCaption,xType) \
  winrtMessageBoxW(xWnd,xText,xCaption,xType)

#ifdef UNICODE
#define MessageBox(xWnd,xText,xCaption,xType) \
  winrtMessageBoxW(xWnd,xText,xCaption,xType)
#else
#define MessageBox(xWnd,xText,xCaption,xType) \
  winrtMessageBoxA(xWnd,xText,xCaption,xType)
#endif /* UNICODE */

#define CreateFileA(xFileName,xDesiredAccess,xShareMode,xSecurityAttributes,xCreationDisposition,xFlagsAndAttributes,xTemplateFile) \
  winrtCreateFileA(xFileName, xDesiredAccess, xShareMode, xSecurityAttributes, xCreationDisposition, xFlagsAndAttributes, xTemplateFile)

#define CreateFileW(xFileName,xDesiredAccess,xShareMode,xSecurityAttributes,xCreationDisposition,xFlagsAndAttributes,xTemplateFile) \
    winrtCreateFileW(xFileName, xDesiredAccess, xShareMode, xSecurityAttributes, xCreationDisposition, xFlagsAndAttributes, xTemplateFile)

#ifdef UNICODE
#define CreateFile(xFileName,xDesiredAccess,xShareMode,xSecurityAttributes,xCreationDisposition,xFlagsAndAttributes,xTemplateFile) \
    winrtCreateFileW(xFileName, xDesiredAccess, xShareMode, xSecurityAttributes, xCreationDisposition, xFlagsAndAttributes, xTemplateFile)
#else
#define CreateFile(xFileName,xDesiredAccess,xShareMode,xSecurityAttributes,xCreationDisposition,xFlagsAndAttributes,xTemplateFile) \
    winrtCreateFileA(xFileName, xDesiredAccess, xShareMode, xSecurityAttributes, xCreationDisposition, xFlagsAndAttributes, xTemplateFile)
#endif /* UNICODE */

#define GetModuleHandleW(xModule) winrtGetModuleHandleW(xModule)
#define GetModuleHandleA(xModule) winrtGetModuleHandleA(xModule)

#ifdef UNICODE
#define GetModuleHandle(xModule) winrtGetModuleHandleW(xModule)
#else
#define GetModuleHandle(xModule) winrtGetModuleHandleA(xModule)
#endif /* UNICODE */

#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

HANDLE WINAPI winrtCreateFileW(
  LPCWSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
);

BOOLEAN winrtRtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
);


int WINAPI winrtMessageBoxW(
  HWND    hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT    uType
);

int WINAPI winrtMessageBoxA(
  HWND    hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT    uType
);

HANDLE WINAPI winrtCreateFileW(
  LPCWSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
);


HANDLE WINAPI winrtCreateFileA(
  LPCSTR                lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
);

HMODULE WINAPI winrtGetModuleHandleW(
  LPCWSTR lpModuleName
);

HMODULE WINAPI winrtGetModuleHandleA(
  LPCSTR lpModuleName
);

#ifdef __cplusplus
  }
#endif /* __cplusplus */
