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
  typedef BYTE uint8_t;
  if (!RandomBuffer) return FALSE;

  auto buffer = Windows::Security::Cryptography::CryptographicBuffer::GenerateRandom(RandomBufferLength);
  if (!buffer) return FALSE;

  auto array = ref new Platform::Array<uint8_t>(RandomBufferLength);
  Windows::Security::Cryptography::CryptographicBuffer::CopyToByteArray(buffer, &array);
  memcpy_s(RandomBuffer, RandomBufferLength, array->Data, RandomBufferLength);
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
static int winuwpInternalMessageBoxPlatformString(
  HWND    hWnd,
  Platform::String ^content,
  Platform::String ^title,
  UINT    uType
)
{
  struct Button
  {
    Button(
           size_t &index,
           Windows::UI::Popups::MessageDialog ^dialog,
           int resultValue,
           const char *name,
           bool showIt,
           bool defaultCommand = false
           )
    {
      if (!showIt) return;

      index_ = index;

      WinUWP::StringConvertToPlatformString str(name);
      command_ = ref new Windows::UI::Popups::UICommand(str.result());
      dialog->Commands->Append(command_);
      if (defaultCommand) dialog->DefaultCommandIndex = static_cast<unsigned int>(index_);

      ++index;
    }

    void clicked(Windows::UI::Popups::IUICommand ^command, int &result) const { if (command_ == command) result = result_; }

    size_t index_ {};
    int result_ {};
    Windows::UI::Popups::IUICommand ^command_ {};
  };

  size_t index {};

  Windows::UI::Popups::MessageDialog ^dialog = (nullptr == title ? ref new Windows::UI::Popups::MessageDialog(content) : ref new Windows::UI::Popups::MessageDialog(content, title));

#define BUTTON_DISPLAY(xType) (((MB_ABORTRETRYIGNORE|MB_CANCELTRYCONTINUE|MB_HELP|MB_OK|MB_OKCANCEL|MB_RETRYCANCEL|MB_YESNO|MB_YESNOCANCEL) & uType) == uType)
#define DEFAULT_BUTTON(xType, xDefValue) (BUTTON_DISPLAY(xType) && (((MB_DEFBUTTON1|MB_DEFBUTTON2|MB_DEFBUTTON3|MB_DEFBUTTON4) & uType) == xDefValue))

  Button helpButton(index, dialog, WM_HELP, "Helper", BUTTON_DISPLAY(MB_HELP));
  Button abortButton(index, dialog, IDABORT, "Abort", BUTTON_DISPLAY(MB_ABORTRETRYIGNORE), DEFAULT_BUTTON(MB_ABORTRETRYIGNORE, MB_DEFBUTTON1));
  Button retryButton(index, dialog, IDRETRY, "Retry", BUTTON_DISPLAY(MB_ABORTRETRYIGNORE) || BUTTON_DISPLAY(MB_RETRYCANCEL), DEFAULT_BUTTON(MB_ABORTRETRYIGNORE, MB_DEFBUTTON2) || DEFAULT_BUTTON(MB_RETRYCANCEL, MB_DEFBUTTON1));
  Button ingoreButton(index, dialog, IDIGNORE, "Ignore", BUTTON_DISPLAY(MB_ABORTRETRYIGNORE), DEFAULT_BUTTON(MB_ABORTRETRYIGNORE, MB_DEFBUTTON3));
  Button okButton(index, dialog, IDOK, "Ok", BUTTON_DISPLAY(MB_OK) || BUTTON_DISPLAY(MB_OKCANCEL), DEFAULT_BUTTON(MB_OK, MB_DEFBUTTON1) || DEFAULT_BUTTON(MB_OKCANCEL, MB_DEFBUTTON1));
  Button yesButton(index, dialog, IDYES, "Yes", BUTTON_DISPLAY(MB_YESNO), DEFAULT_BUTTON(MB_YESNO, MB_DEFBUTTON1));
  Button noButton(index, dialog, IDNO, "No", BUTTON_DISPLAY(MB_YESNO), DEFAULT_BUTTON(MB_YESNO, MB_DEFBUTTON2));
  Button cancelButton(index, dialog, IDCANCEL, "Cancel",
    BUTTON_DISPLAY(MB_CANCELTRYCONTINUE) || BUTTON_DISPLAY(MB_RETRYCANCEL) || BUTTON_DISPLAY(MB_YESNOCANCEL),
    DEFAULT_BUTTON(MB_CANCELTRYCONTINUE, MB_DEFBUTTON1) || DEFAULT_BUTTON(MB_RETRYCANCEL, MB_DEFBUTTON2) || DEFAULT_BUTTON(MB_YESNOCANCEL, MB_DEFBUTTON3));
  Button tryAgainButton(index, dialog, IDTRYAGAIN, "Try Again", BUTTON_DISPLAY(MB_CANCELTRYCONTINUE), DEFAULT_BUTTON(MB_CANCELTRYCONTINUE, MB_DEFBUTTON2));
  Button continueButton(index, dialog, IDCONTINUE, "Continue", BUTTON_DISPLAY(MB_CANCELTRYCONTINUE), DEFAULT_BUTTON(MB_CANCELTRYCONTINUE, MB_DEFBUTTON3));

  auto asyncOperation = dialog->ShowAsync();

  int result { IDOK };

  concurrency::create_task(asyncOperation).then([](concurrency::task<Windows::UI::Popups::IUICommand^> task) {}).wait();

  Windows::UI::Popups::IUICommand ^clicked = asyncOperation->GetResults();

  helpButton.clicked(clicked, result);
  abortButton.clicked(clicked, result);
  retryButton.clicked(clicked, result);
  ingoreButton.clicked(clicked, result);
  okButton.clicked(clicked, result);
  yesButton.clicked(clicked, result);
  noButton.clicked(clicked, result);
  cancelButton.clicked(clicked, result);
  tryAgainButton.clicked(clicked, result);
  continueButton.clicked(clicked, result);

  return result;
}

//-----------------------------------------------------------------------------
static int winuwpInternalMessageBoxA(
  HWND    hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT    uType
)
{
  WinUWP::StringConvertToPlatformString title(lpCaption);
  WinUWP::StringConvertToPlatformString content(lpText);
  return winuwpInternalMessageBoxPlatformString(hWnd, content.result(), title.result(), uType);
}

//-----------------------------------------------------------------------------
static int winuwpInternalMessageBoxW(
  HWND    hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT    uType
)
{
  WinUWP::StringConvertToPlatformString title(lpCaption);
  WinUWP::StringConvertToPlatformString content(lpText);
  return winuwpInternalMessageBoxPlatformString(hWnd, content.result(), title.result(), uType);
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
static DWORD winuwpInternalGetTempPathW(
  DWORD  nBufferLength,
  LPWSTR lpBuffer
)
{
#ifdef WIN10
  return GetTempPathW(nBufferLength, lpBuffer);
#else /* WIN10 */
  auto current = Windows::Storage::ApplicationData::Current;
  if (!current) return 0;

  auto localFolder = current->TemporaryFolder;
  if (!localFolder) return 0;

  auto folder = localFolder->Path;
  if (!folder) return 0;

  WinUWP::StringConvertToUTF16 str(folder);
  if (static_cast<size_t>(nBufferLength) < str.length()) return static_cast<DWORD>(str.length()+1);

  str.result(lpBuffer, static_cast<size_t>(nBufferLength));
  return static_cast<DWORD>(str.length());
#endif /* WIN10 */
}

//-----------------------------------------------------------------------------
static DWORD winuwpInternalGetTempPathA(
  DWORD  nBufferLength,
  LPSTR lpBuffer
)
{
  auto sizeNeeded = winuwpInternalGetTempPathW(0, NULL);
  if (0 == sizeNeeded) return 0;

  auto tempBuffer = (wchar_t *)malloc((sizeNeeded + 1) * sizeof(wchar_t));
  memset(tempBuffer, 0, (sizeNeeded + 1) * sizeof(wchar_t));

  auto recheckSize = winuwpInternalGetTempPathW(sizeNeeded, tempBuffer);
  if (sizeNeeded != recheckSize) {
    free(tempBuffer);
    tempBuffer = NULL;
    return 0;
  }

  WinUWP::StringConvertToUTF8 conv(static_cast<const wchar_t *>(tempBuffer));
  free(tempBuffer);
  tempBuffer = NULL;

  if ((NULL == lpBuffer) || (conv.length() > nBufferLength)) return static_cast<DWORD>(conv.length()+1);

  conv.result(lpBuffer, static_cast<size_t>(nBufferLength));
  return static_cast<DWORD>(conv.length());
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

//-----------------------------------------------------------------------------
static HANDLE winuwpInternalCreateEventW(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCWSTR               lpName
)
{
  return CreateEventEx(lpEventAttributes, lpName, (bManualReset ? CREATE_EVENT_MANUAL_RESET : 0) | (bInitialState ? CREATE_EVENT_INITIAL_SET : 0), EVENT_ALL_ACCESS);
}

//-----------------------------------------------------------------------------
static HANDLE winuwpInternalCreateEventA(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCSTR                lpName
)
{
  return winuwpInternalCreateEventW(lpEventAttributes, bManualReset, bInitialState, WinUWP::StringConvertToUTF16(lpName).result());
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
int winuwpMessageBoxW(
  HWND    hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT    uType
)
{
  return winuwpInternalMessageBoxW(hWnd, lpText, lpCaption, uType);
}

//-----------------------------------------------------------------------------
int winuwpMessageBoxA(
  HWND    hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT    uType
)
{
  return winuwpInternalMessageBoxA(hWnd,lpText,lpCaption, uType);
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
DWORD winuwpGetTempPathW(
  DWORD  nBufferLength,
  LPWSTR lpBuffer
)
{
  return winuwpInternalGetTempPathW(nBufferLength, lpBuffer);
}

//-----------------------------------------------------------------------------
DWORD winuwpGetTempPathA(
  DWORD  nBufferLength,
  LPSTR lpBuffer
)
{
  return winuwpInternalGetTempPathA(nBufferLength, lpBuffer);
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
DWORD winuwpWaitForSingleObject(
  HANDLE hHandle,
  DWORD  dwMilliseconds
)
{
  return WaitForSingleObjectEx(hHandle, dwMilliseconds, FALSE);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
DWORD winuwpWaitForMultipleObjects(
  DWORD  nCount,
  const HANDLE *lpHandles,
  BOOL   bWaitAll,
  DWORD  dwMilliseconds
)
{
  return WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, FALSE);
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

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void winuwpInitializeCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
)
{
  InitializeCriticalSectionEx(lpCriticalSection, 0, 0);
}

//-----------------------------------------------------------------------------
HANDLE winuwpCreateEventW(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCWSTR               lpName
)
{
  return winuwpInternalCreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
HANDLE winuwpCreateEventA(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCSTR               lpName
)
{
  return winuwpInternalCreateEventA(lpEventAttributes, bManualReset, bInitialState, lpName);
}


#ifdef __cplusplus
  }
#endif /* __cplusplus */

#endif /* WINUWP */

void winuwp_compat_win_noop() {}
