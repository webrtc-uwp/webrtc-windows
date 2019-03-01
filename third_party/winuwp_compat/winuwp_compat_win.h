/*
* This header is injected using /FI cl.exe flag for winuwp project.
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

#include <winapifamily.h>

#ifdef WINUWP

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
#if defined(UNICODE) || defined(_UNICODE)
#ifndef UNICODE
#define WUNUWP_COMPAT_DID_DEFINE_UNICODE
#define UNICODE
#endif /* ndef UNICODE */
#endif /* defined(UNICODE) || defined(_UNICODE) */

#ifndef CONST
#define WUNUWP_COMPAT_DID_DEFINE_CONST
#define CONST const
#endif /* CONST */

#ifndef __nullterminated 
#define WUNUWP_COMPAT_DID_DEFINE___nullterminated
#define __nullterminated 
#endif /* __nullterminated */

#ifndef FALSE
#define WUNUWP_COMPAT_DID_DEFINE_FALSE
#define FALSE 0
#endif /* FALSE */

#ifndef TRUE
#define WUNUWP_COMPAT_DID_DEFINE_TRUE
#define TRUE 1
#endif /* TRUE */

#ifndef STRICT
#ifndef NO_STRICT
#define WUNUWP_COMPAT_DID_DEFINE_STRICT
#define STRICT 1
#endif /* ndef NO_STRICT */
#endif /* STRICT */

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef void *PVOID;
typedef PVOID HANDLE;

#ifndef DECLARE_HANDLE
#define WUNUWP_COMPAT_DID_DEFINE_DECLARE_HANDLE

#ifdef STRICT
#if 0 && (_MSC_VER > 1000)
#define DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name
#else /* 0 && (_MSC_VER > 1000) */
#define WUNUWP_COMPAT_DID_DEFINE_DECLARE_HANDLE
#define DECLARE_HANDLE(name) struct name##__; typedef struct name##__ *name
#endif /* 0 && (_MSC_VER > 1000) */

#else /* STRICT */
#define DECLARE_HANDLE(name) typedef HANDLE name
#endif /* STRICT */

#endif /* ndef DECLARE_HANDLE */

typedef unsigned char BYTE;
typedef int BOOL;
typedef BYTE BOOLEAN;
typedef char CHAR;

#ifndef _NATIVE_WCHAR_T_DEFINED
typedef unsigned short wchar_t;
#endif //_NATIVE_WCHAR_T_DEFINED

typedef wchar_t WCHAR;
typedef unsigned long ULONG;
typedef unsigned int UINT;
typedef unsigned long DWORD;
typedef DWORD LCID;
typedef void *LPVOID;
typedef PVOID HANDLE;
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HINSTANCE);
typedef HINSTANCE HMODULE;
typedef CHAR *LPSTR;
typedef __nullterminated CONST CHAR *LPCSTR;
typedef WCHAR *LPWSTR;
typedef CONST WCHAR *LPCWSTR;

typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;
typedef SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;

typedef struct _RTL_CRITICAL_SECTION CRITICAL_SECTION;
typedef CRITICAL_SECTION *LPCRITICAL_SECTION;

typedef struct _SYSTEMTIME SYSTEMTIME;
typedef SYSTEMTIME *PSYSTEMTIME;

#ifdef UNICODE
 typedef LPWSTR LPTSTR;
 typedef LPCWSTR LPCTSTR; 
#else /* UNICODE */
 typedef LPSTR LPTSTR;
 typedef LPCSTR LPCTSTR;
#endif /* UNICODE */

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef RtlGenRandom
#undef RtlGenRandom
#endif /* RtlGenRandom */

BOOLEAN winuwpRtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
);

inline BOOLEAN RtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
)
{
  return winuwpRtlGenRandom(RandomBuffer, RandomBufferLength);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#define GetDateFormatW(xLocale,xFlags,xDate,xFormat,xDateStr,xccDate) winuwpGetDateFormatW(xLocale,xFlags,xDate,xFormat,xDateStr,xccDate)
#define GetDateFormatA(xLocale,xFlags,xDate,xFormat,xDateStr,xccDate) winuwpGetDateFormatA(xLocale,xFlags,xDate,xFormat,xDateStr,xccDate)

int winuwpGetDateFormatW(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpDate,
  LPCWSTR    lpFormat,
  LPWSTR     lpDateStr,
  int        cchDate
);

int winuwpGetDateFormatA(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpDate,
  LPCSTR     lpFormat,
  LPSTR      lpDateStr,
  int        cchDate
);

inline int GetDateFormat(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpDate,
  LPCTSTR    lpFormat,
  LPTSTR     lpDateStr,
  int        cchDate
)
{
#ifdef UNICODE
  return winuwpGetDateFormatW(Locale,dwFlags,lpDate,lpFormat,lpDateStr,cchDate);
#else
  return winuwpGetDateFormatA(Locale,dwFlags,lpDate,lpFormat,lpDateStr,cchDate);
#endif /* UNICODE */
}


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */


#define GetTimeFormatW(xLocale,xFlags,xTime,xFormat,xTimeStr,xccTime) winuwpGetTimeFormatW(xLocale,xFlags,xTime,xFormat,xTimeStr,xccTime)
#define GetTimeFormatA(xLocale,xFlags,xTime,xFormat,xTimeStr,xccTime) winuwpGetTimeFormatA(xLocale,xFlags,xTime,xFormat,xTimeStr,xccTime)

int winuwpGetTimeFormatW(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpTime,
  LPCWSTR    lpFormat,
  LPWSTR     lpTimeStr,
  int        cchTime
);

int winuwpGetTimeFormatA(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpTime,
  LPCSTR    lpFormat,
  LPSTR     lpTimeStr,
  int        cchTime
);

inline int GetTimeFormat(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpTime,
  LPCTSTR    lpFormat,
  LPTSTR     lpTimeStr,
  int        cchTime
)
{
#ifdef UNICODE
  return winuwpGetTimeFormatW(Locale,dwFlags,lpTime,lpFormat,lpTimeStr,cchTime);
#else
  return winuwpGetTimeFormatA(Locale,dwFlags,lpTime,lpFormat,lpTimeStr,cchTime);
#endif /* UNICODE */
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef CreateFile
#undef CreateFile
#endif /* CreateFile */

#define CreateFileW(xFileName,xDesiredAccess,xShareMode,xSecurityAttributes,xCreationDisposition,xFlagsAndAttributes,xTemplateFile) winuwpCreateFileW(xFileName, xDesiredAccess, xShareMode, xSecurityAttributes, xCreationDisposition, xFlagsAndAttributes, xTemplateFile)
#define CreateFileA(xFileName,xDesiredAccess,xShareMode,xSecurityAttributes,xCreationDisposition,xFlagsAndAttributes,xTemplateFile) winuwpCreateFileA(xFileName, xDesiredAccess, xShareMode, xSecurityAttributes, xCreationDisposition, xFlagsAndAttributes, xTemplateFile)

HANDLE winuwpCreateFileW(
  LPCWSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
);

HANDLE winuwpCreateFileA(
  LPCSTR                lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
);

inline HANDLE CreateFile(
  LPCTSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
#ifdef UNICODE
  return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#else
  return CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
#endif /* UNICODE */
}


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef MoveFile
#undef MoveFile
#endif /* MoveFile */

#define MoveFileW(xExistingFileName,xNewFileName) winuwpMoveFileW(xExistingFileName,xNewFileName)
#define MoveFileA(xExistingFileName,xNewFileName) winuwpMoveFileA(xExistingFileName,xNewFileName)


BOOL winuwpMoveFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName
);

BOOL winuwpMoveFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName
);

inline BOOL MoveFile(
  LPCTSTR lpExistingFileName,
  LPCTSTR lpNewFileName
)
{
#ifdef UNICODE
  return winuwpMoveFileW(lpExistingFileName, lpNewFileName);
#else
  return winuwpMoveFileA(lpExistingFileName, lpNewFileName);
#endif /* UNICODE */
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef CopyFile
#undef CopyFile
#endif /* CopyFile */

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#define CopyFileW(xExistingFileName,xNewFileName,xFailIfExistse) winuwpCopyFileW(xExistingFileName,xNewFileName,xFailIfExistse)
#define CopyFileA(xExistingFileName,xNewFileName,xFailIfExistse) winuwpCopyFileA(xExistingFileName,xNewFileName,xFailIfExistse)
#endif
BOOL winuwpCopyFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName,
  BOOL    bFailIfExists
);

BOOL winuwpCopyFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName,
  BOOL    bFailIfExists
);

inline BOOL CopyFile(
  LPCTSTR lpExistingFileName,
  LPCTSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
#ifdef UNICODE
  return winuwpCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
#else
  return winuwpCopyFileA(lpExistingFileName, lpNewFileName, bFailIfExists);
#endif /* UNICODE */
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#define CommandLineToArgvW(xCmdLine, xNumArgs) winuwpCommandLineToArgvW(xCmdLine, xNumArgs)
#define CommandLineToArgvA(xCmdLine, xNumArgs) winuwpCommandLineToArgvA(xCmdLine, xNumArgs)

LPWSTR* winuwpCommandLineToArgvW(
  LPCWSTR lpCmdLine,
  int     *pNumArgs
);


LPSTR* winuwpCommandLineToArgvA(
  LPCSTR lpCmdLine,
  int    *pNumArgs
);

inline LPTSTR* CommandLineToArgv(
  LPCTSTR lpCmdLine,
  int     *pNumArgs
)
{
#ifdef UNICODE
  return winuwpCommandLineToArgvW(lpCmdLine, pNumArgs);
#else
  return winuwpCommandLineToArgvA(lpCmdLine, pNumArgs);
#endif /* UNICODE */
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef GetModuleHandle
#undef GetModuleHandle
#endif /* GetModuleHandle */

#define GetModuleHandleW(xModule) winuwpGetModuleHandleW(xModule)
#define GetModuleHandleA(xModule) winuwpGetModuleHandleA(xModule)

HMODULE winuwpGetModuleHandleW(
  LPCWSTR lpModuleName
);

HMODULE winuwpGetModuleHandleA(
  LPCSTR lpModuleName
);

#ifdef GetModuleHandle
#undef GetModuleHandle
#endif /* GetModuleHandle */

inline HMODULE GetModuleHandle(
  LPCTSTR lpModuleName
)
{
#ifdef UNICODE
  return winuwpGetModuleHandleW(lpModuleName);
#else
  return winuwpGetModuleHandleA(lpModuleName);
#endif /* UNICODE */
}


#ifdef __cplusplus
  }
#endif /* __cplusplus */


/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

#ifdef WUNUWP_COMPAT_DID_DEFINE_UNICODE
#undef WUNUWP_COMPAT_DID_DEFINE_UNICODE
#undef UNICODE
#endif /* WUNUWP_COMPAT_DID_DEFINE_UNICODE */

#ifdef WUNUWP_COMPAT_DID_DEFINE_CONST
#undef WUNUWP_COMPAT_DID_DEFINE_CONST
#undef CONST
#endif /* WUNUWP_COMPAT_DID_DEFINE_CONST */

#ifdef WUNUWP_COMPAT_DID_DEFINE___nullterminated
#undef WUNUWP_COMPAT_DID_DEFINE___nullterminated
#undef __nullterminated 
#endif /* WUNUWP_COMPAT_DID_DEFINE___nullterminated */

#ifdef WUNUWP_COMPAT_DID_DEFINE_FALSE
#undef WUNUWP_COMPAT_DID_DEFINE_FALSE
#undef FALSE
#endif /* WUNUWP_COMPAT_DID_DEFINE_FALSE */

#ifdef WUNUWP_COMPAT_DID_DEFINE_TRUE
#undef WUNUWP_COMPAT_DID_DEFINE_TRUE
#undef TRUE
#endif /* WUNUWP_COMPAT_DID_DEFINE_TRUE */

#ifdef WUNUWP_COMPAT_DID_DEFINE_STRICT
#undef WUNUWP_COMPAT_DID_DEFINE_STRICT
#undef STRICT
#endif /* WUNUWP_COMPAT_DID_DEFINE_STRICT */

#ifdef WUNUWP_COMPAT_DID_DEFINE_DECLARE_HANDLE
#undef WUNUWP_COMPAT_DID_DEFINE_DECLARE_HANDLE
#undef DECLARE_HANDLE
#endif /* WUNUWP_COMPAT_DID_DEFINE_DECLARE_HANDLE */

#endif /* WINUWP */
