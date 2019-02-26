/*
* This source is for use with injected /FI cl.exe header for winuwp project.
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

/*
 This header is injected using /FI cl.exe flag for yasm project.
*/

#include "winuwp_compat_win.h"
#include "winuwp_compat_internal.h"

#ifdef WINUWP

#include <Windows.h>

#include <ppltasks.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <bcrypt.h>

#define UWP_COMPAT_NT_INFORMATION(xStatus) \
  (((xStatus) >= 0x40000000) && ((xStatus) <= 0x7FFFFFFF))

#define UWP_COMPAT_NT_SUCCESS(xStatus) \
  ((((xStatus) >= 0) && ((xStatus) <= 0x3FFFFFFF)) || UWP_COMPAT_NT_INFORMATION(xStatus))

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static BOOLEAN winuwpInternalRtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
)
{
  auto status = BCryptGenRandom(NULL, (PUCHAR)RandomBuffer, RandomBufferLength, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
  if (!UWP_COMPAT_NT_SUCCESS(status))
    return FALSE;
  return TRUE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static int winuwpInternalGetDateFormatW(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpDate,
  LPCWSTR    lpFormat,
  LPWSTR     lpDateStr,
  int        cchDate
)
{
  wchar_t buffer[LOCALE_NAME_MAX_LENGTH];
  memset(&(buffer[0]), 0, sizeof(buffer));

  auto result = LCIDToLocaleName(Locale, buffer, LOCALE_NAME_MAX_LENGTH, 0);
  if (!result) return 0;

  return GetDateFormatEx(buffer, dwFlags, lpDate, lpFormat, lpDateStr, cchDate, NULL);
}

//-----------------------------------------------------------------------------
static int winuwpInternalGetDateFormatA(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpDate,
  LPCSTR     lpFormat,
  LPSTR      lpDateStr,
  int        cchDate
)
{
  WinUWP::StringConvertToUTF16 format(lpFormat);

  auto sizeNeeded = winuwpGetDateFormatW(Locale, dwFlags, lpDate, format.result(), NULL, 0);
  if (0 == sizeNeeded) return 0;

  wchar_t *buffer = (wchar_t *)malloc(sizeof(wchar_t)*(sizeNeeded+1));
  memset(buffer, 0, sizeof(wchar_t)*(sizeNeeded+1));

  auto recheckSize = winuwpGetDateFormatW(Locale, dwFlags, lpDate, format.result(), buffer, sizeNeeded+1);
  if (0 == recheckSize) {
    free(buffer);
    return 0;
  }

  WinUWP::StringConvertToUTF8 conv(buffer);
  free(buffer);

  if ((!lpDateStr) ||
      ((static_cast<size_t>(cchDate)) < conv.length())) {
    if (0 != cchDate) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }
    return static_cast<int>(conv.length()+1);
  }

  conv.result(lpDateStr, cchDate);

  return static_cast<int>(conv.length()+1);
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static int winuwpInternalGetTimeFormatW(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpTime,
  LPCWSTR    lpFormat,
  LPWSTR     lpTimeStr,
  int        cchTime
)
{
  wchar_t buffer[LOCALE_NAME_MAX_LENGTH];
  memset(&(buffer[0]), 0, sizeof(buffer));

  auto result = LCIDToLocaleName(Locale, buffer, LOCALE_NAME_MAX_LENGTH, 0);
  if (!result) return 0;

  return GetTimeFormatEx(buffer, dwFlags, lpTime, lpFormat, lpTimeStr, cchTime);
}

//-----------------------------------------------------------------------------
static int winuwpInternalGetTimeFormatA(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpTime,
  LPCSTR     lpFormat,
  LPSTR      lpTimeStr,
  int        cchTime
)
{
 WinUWP::StringConvertToUTF16 format(lpFormat);

  auto sizeNeeded = winuwpGetDateFormatW(Locale, dwFlags, lpTime, format.result(), NULL, 0);
  if (0 == sizeNeeded) return 0;

  wchar_t *buffer = (wchar_t *)malloc(sizeof(wchar_t)*(sizeNeeded+1));
  memset(buffer, 0, sizeof(wchar_t)*(sizeNeeded+1));

  auto recheckSize = winuwpGetDateFormatW(Locale, dwFlags, lpTime, format.result(), buffer, sizeNeeded+1);
  if (0 == recheckSize) {
    free(buffer);
    return 0;
  }

  WinUWP::StringConvertToUTF8 conv(buffer);
  free(buffer);

  if ((!lpTimeStr) ||
      ((static_cast<size_t>(cchTime)) < conv.length())) {
    if (0 != cchTime) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }
    return static_cast<int>(conv.length()+1);
  }

  conv.result(lpTimeStr, cchTime);

  return static_cast<int>(conv.length()+1);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static HANDLE winuwpInternalCreateFileW(
  LPCWSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
  CREATEFILE2_EXTENDED_PARAMETERS params;
  memset(&params, 0, sizeof(params));
  params.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);

  DWORD filter = FILE_ATTRIBUTE_ARCHIVE |
    FILE_ATTRIBUTE_ENCRYPTED |
    FILE_ATTRIBUTE_HIDDEN |
    FILE_ATTRIBUTE_INTEGRITY_STREAM |
    FILE_ATTRIBUTE_NORMAL |
    FILE_ATTRIBUTE_OFFLINE |
    FILE_ATTRIBUTE_READONLY |
    FILE_ATTRIBUTE_SYSTEM |
    FILE_ATTRIBUTE_TEMPORARY;

  DWORD attribs = dwFlagsAndAttributes & (filter);
  DWORD flags = (dwFlagsAndAttributes | filter) ^ filter;

  params.dwFileAttributes = attribs;
  params.dwFileFlags = flags;
  params.lpSecurityAttributes = lpSecurityAttributes;
  params.hTemplateFile = hTemplateFile;

  return CreateFile2(
    lpFileName,
    dwDesiredAccess,
    dwShareMode,
    dwCreationDisposition,
    &params
  );
}

//-----------------------------------------------------------------------------
static HANDLE winuwpInternalCreateFileA(
  LPCSTR                lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
  return winuwpInternalCreateFileW(WinUWP::StringConvertToUTF16(lpFileName).result(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static BOOL winuwpInternalMoveFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName
)
{
  return MoveFileExW(lpExistingFileName, lpNewFileName, 0);
}

//-----------------------------------------------------------------------------
static BOOL winuwpInternalMoveFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName
)
{
  return MoveFileExA(lpExistingFileName, lpNewFileName, 0);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static BOOL winuwpInternalCopyFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
  COPYFILE2_EXTENDED_PARAMETERS params {};
  params.dwCopyFlags = (bFailIfExists ? COPY_FILE_FAIL_IF_EXISTS : 0);
  return CopyFile2(lpExistingFileName, lpNewFileName, &params);
}

//-----------------------------------------------------------------------------
static BOOL winuwpInternalCopyFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
  WinUWP::StringConvertToUTF16 existingFile(lpExistingFileName);
  WinUWP::StringConvertToUTF16 newFile(lpNewFileName);
  return winuwpInternalCopyFileW(existingFile.result(), newFile.result(), bFailIfExists);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

template <typename char_type>
static char_type ** winuwpInternalCommandLineToArgv(
  const char_type* lpCmdLine,
  int    *pNumArgs
)
{
  // NOTE: see https://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft(v=vs.85).aspx

  if (!lpCmdLine) {
    SetLastError(ERROR_INVALID_PARAMETER);
    return NULL;
  }
  if (NULL != pNumArgs) {
    *pNumArgs = 0;
  }

  size_t maxNumElements {};
  size_t len {};

  // scope: count number of elements
  {
    bool lastWasSpace {false};
    const char_type *pos = lpCmdLine;
    for (; 0 != *pos; ++pos, ++len) {
      if ((static_cast<char_type>(' ') != *pos) &&
          (static_cast<char_type>('\t') != *pos)) {
        lastWasSpace = false;
        continue;
      }

      if (lastWasSpace) continue;

      lastWasSpace = true;
      ++maxNumElements;
    }
  }

  // allocate enough space for array of pointers, nul terminating character for each array + 1 (double nul at end for safety), and length of string
  void *buffer = LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, ((sizeof(char_type*)*(maxNumElements+1))) + ((sizeof(char_type)*(maxNumElements+1)) + (sizeof(char_type)*len)));
  if (!buffer) {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return NULL;
  }

  char_type **array = reinterpret_cast<char_type **>(buffer);
  char_type **result = array;

  char_type *dest = reinterpret_cast<char_type *>((reinterpret_cast<BYTE *>(buffer) + (sizeof(char_type *)*(maxNumElements+1))));
  const char_type *pos = lpCmdLine;

  size_t totalElements {0};
  while (0 != *pos) {
    // setup next array
    array[totalElements] = dest;
    ++totalElements;

    bool insideQuotes {false};

    while (0 != *pos) {
      if (!insideQuotes) {
        if ((static_cast<char_type>(' ') == (*pos)) ||
            (static_cast<char_type>('\t') == (*pos))) {
          ++pos;
          continue;
        }
      }

      if (static_cast<char_type>('\\') == (*pos)) {
        // special case, need to count number of backslashed
        size_t totalBackslashes = 0;
        while (static_cast<char_type>('\\') == (*pos)) {
          ++totalBackslashes;
          ++pos;
        }
        bool nextIsQuote = (static_cast<char_type>('\"') == (*pos));
        if (nextIsQuote) {
          ++pos; // skip over double quotes
          if (0 == (totalBackslashes % 2)) {
            // As per reference:
            // If an even number of backslashes is followed by a double quotation mark,
            // one backslash is placed in the argv array for every pair of backslashes,
            // and the double quotation mark is interpreted as a string delimiter.
            size_t half = totalBackslashes / 2;
            for (size_t loop = 0; loop < half; ++loop, ++dest) {
              *dest = static_cast<char_type>('\\');
            }
            insideQuotes = !insideQuotes;
          } else {
            // As per reference:
            // If an odd number of backslashes is followed by a double quotation mark,
            // one backslash is placed in the argv array for every pair of backslashes,
            // and the double quotation mark is "escaped" by the remaining backslash,
            // causing a literal double quotation mark (") to be placed in argv.
            size_t half = (totalBackslashes-1) / 2;
            for (size_t loop = 0; loop < half; ++loop, ++dest) {
              *dest = static_cast<char_type>('\\');
            }
            *dest = static_cast<char_type>('\"');
            ++dest;
          }
        } else {
          // As per reference:
          // Backslashes are interpreted literally, unless they immediately
          // precede a double quotation mark.
          for (size_t loop = 0; loop < totalBackslashes; ++loop, ++dest) {
            *dest = static_cast<char_type>('\\');
          }
        }
        continue;
      }
      if (static_cast<char_type>('\"') == (*pos)) {
        insideQuotes = !insideQuotes;
        ++pos;
        continue;
      }
    }
    
    while ((0 != *pos) &&
           ((static_cast<char_type>(' ') == (*pos)) ||
            (static_cast<char_type>('\t') == (*pos)))) {
      // skip over delimited white space
      ++pos;
    }

    // skip over nul terminator in dest
    ++dest;
  }

  if (NULL != pNumArgs) {
    *pNumArgs = static_cast<int>(totalElements);
  }
  return result;
}

//-----------------------------------------------------------------------------
static LPSTR* winuwpInternalCommandLineToArgvA(
  LPCSTR lpCmdLine,
  int    *pNumArgs
)
{
  return winuwpInternalCommandLineToArgv<char>(lpCmdLine, pNumArgs);
}

//-----------------------------------------------------------------------------
static LPWSTR* winuwpInternalCommandLineToArgvW(
  LPCWSTR lpCmdLine,
  int     *pNumArgs
)
{
  return winuwpInternalCommandLineToArgv<wchar_t>(lpCmdLine, pNumArgs);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
static HMODULE winuwpInternalGetModuleHandleW(
  LPCWSTR lpModuleName
)
{
  return LoadPackagedLibrary(lpModuleName, 0);
}

//-----------------------------------------------------------------------------
static HMODULE winuwpInternalGetModuleHandleA(
  LPCSTR lpModuleName
)
{
  return winuwpInternalGetModuleHandleW(WinUWP::StringConvertToUTF16(lpModuleName).result());
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
BOOLEAN winuwpRtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
)
{
  return winuwpInternalRtlGenRandom(RandomBuffer,RandomBufferLength);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int winuwpGetDateFormatW(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpDate,
  LPCWSTR    lpFormat,
  LPWSTR     lpDateStr,
  int        cchDate
)
{
  return winuwpInternalGetDateFormatW(Locale,dwFlags,lpDate,lpFormat,lpDateStr,cchDate);
}

//-----------------------------------------------------------------------------
int winuwpGetDateFormatA(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpDate,
  LPCSTR    lpFormat,
  LPSTR     lpDateStr,
  int        cchDate
)
{
  return winuwpInternalGetDateFormatA(Locale,dwFlags,lpDate,lpFormat,lpDateStr,cchDate);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
int winuwpGetTimeFormatW(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpTime,
  LPCWSTR    lpFormat,
  LPWSTR     lpTimeStr,
  int        cchTime
)
{
  return winuwpInternalGetTimeFormatW(Locale,dwFlags,lpTime,lpFormat,lpTimeStr,cchTime);
}

//-----------------------------------------------------------------------------
int winuwpGetTimeFormatA(
  LCID       Locale,
  DWORD      dwFlags,
  const SYSTEMTIME *lpTime,
  LPCSTR     lpFormat,
  LPSTR      lpTimeStr,
  int        cchTime
)
{
  return winuwpInternalGetTimeFormatA(Locale,dwFlags,lpTime,lpFormat,lpTimeStr,cchTime);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
HANDLE winuwpCreateFileW(
  LPCWSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
  return winuwpInternalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//-----------------------------------------------------------------------------
HANDLE winuwpCreateFileA(
  LPCSTR                lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
  return winuwpInternalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
BOOL winuwpMoveFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName
)
{
  return winuwpInternalMoveFileW(lpExistingFileName, lpNewFileName);
}

//-----------------------------------------------------------------------------
BOOL winuwpMoveFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName
)
{
  return winuwpInternalMoveFileA(lpExistingFileName, lpNewFileName);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
BOOL winuwpCopyFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
  return winuwpInternalCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

//-----------------------------------------------------------------------------
BOOL winuwpCopyFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
  return winuwpInternalCopyFileA(lpExistingFileName, lpNewFileName, bFailIfExists);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
LPWSTR* winuwpCommandLineToArgvW(
  LPCWSTR lpCmdLine,
  int     *pNumArgs
)
{
  return winuwpInternalCommandLineToArgvW(lpCmdLine, pNumArgs);
}

//-----------------------------------------------------------------------------
LPSTR* winuwpCommandLineToArgvA(
  LPCSTR lpCmdLine,
  int    *pNumArgs
)
{  
  return winuwpInternalCommandLineToArgvA(lpCmdLine, pNumArgs);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
HMODULE winuwpGetModuleHandleW(
  LPCWSTR lpModuleName
)
{
  return winuwpInternalGetModuleHandleW(lpModuleName);
}

//-----------------------------------------------------------------------------
HMODULE winuwpGetModuleHandleA(
  LPCSTR lpModuleName
)
{
  return winuwpInternalGetModuleHandleA(lpModuleName);
}


#ifdef __cplusplus
  }
#endif /* __cplusplus */

#endif /* WINUWP */

void winuwp_compat_win_noop() {}
