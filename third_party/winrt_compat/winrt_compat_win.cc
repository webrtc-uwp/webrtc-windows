/*
* This source is for use with injected /FI cl.exe header for winrt project.
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

#include "winrt_compat_win.h"
#include "winrt_compat_internal.h"

#include <Windows.h>

#include <ppltasks.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

//-----------------------------------------------------------------------------
static BOOLEAN winrtInternalRtlGenRandom(
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
static int WINAPI winrtInternalMessageBoxPlatformString(
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

      WinRT::StringConvertToPlatformString str(name);
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
static int WINAPI winrtInternalMessageBoxA(
  HWND    hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT    uType
)
{
  WinRT::StringConvertToPlatformString title(lpCaption);
  WinRT::StringConvertToPlatformString content(lpText);
  return winrtInternalMessageBoxPlatformString(hWnd, content.result(), title.result(), uType);
}

//-----------------------------------------------------------------------------
static int WINAPI winrtInternalMessageBoxW(
  HWND    hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT    uType
)
{
  WinRT::StringConvertToPlatformString title(lpCaption);
  WinRT::StringConvertToPlatformString content(lpText);
  return winrtInternalMessageBoxPlatformString(hWnd, content.result(), title.result(), uType);
}

//-----------------------------------------------------------------------------
static int WINAPI winrtInternalMessageBoxT(
  HWND    hWnd,
  LPCTSTR lpText,
  LPCTSTR lpCaption,
  UINT    uType
)
{
  WinRT::StringConvertToPlatformString title(lpCaption);
  WinRT::StringConvertToPlatformString content(lpText);
  return winrtInternalMessageBoxPlatformString(hWnd, content.result(), title.result(), uType);
}

//-----------------------------------------------------------------------------
static HANDLE WINAPI winrtInternalCreateFileW(
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
static HANDLE WINAPI winrtInternalCreateFileA(
  LPCSTR                lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
  return winrtInternalCreateFileW(WinRT::StringConvertToUTF16(lpFileName).result(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//-----------------------------------------------------------------------------
DWORD WINAPI winrtInternalGetTempPathW(
  DWORD  nBufferLength,
  LPWSTR lpBuffer
)
{
  return GetTempPathW(nBufferLength, lpBuffer);
}

//-----------------------------------------------------------------------------
DWORD WINAPI winrtInternalGetTempPathA(
  DWORD  nBufferLength,
  LPSTR lpBuffer
)
{
  auto sizeNeeded = winrtInternalGetTempPathW(0, NULL);
  if (0 == sizeNeeded) return 0;

  auto tempBuffer = (wchar_t *)malloc((sizeNeeded + 1) * sizeof(wchar_t));
  memset(tempBuffer, 0, (sizeNeeded + 1) * sizeof(wchar_t));

  auto recheckSize = winrtInternalGetTempPathW(sizeNeeded, tempBuffer);
  if (sizeNeeded != recheckSize) {
    free(tempBuffer);
    tempBuffer = NULL;
    return 0;
  }

  WinRT::StringConvertToUTF8 conv(static_cast<const wchar_t *>(tempBuffer));
  free(tempBuffer);
  tempBuffer = NULL;

  if ((NULL == lpBuffer) || (conv.length() > nBufferLength)) return static_cast<DWORD>(conv.length());

  conv.result(lpBuffer, static_cast<size_t>(nBufferLength));
  return static_cast<DWORD>(conv.length());
}

//-----------------------------------------------------------------------------
BOOL WINAPI winrtInternalMoveFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName
)
{
  return MoveFileExW(lpExistingFileName, lpNewFileName, 0);
}

//-----------------------------------------------------------------------------
BOOL WINAPI winrtInternalMoveFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName
)
{
  return MoveFileExA(lpExistingFileName, lpNewFileName, 0);
}

//-----------------------------------------------------------------------------
BOOL WINAPI winrtInternalCopyFileW(
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
BOOL WINAPI winrtInternalCopyFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
  WinRT::StringConvertToUTF16 existingFile(lpExistingFileName);
  WinRT::StringConvertToUTF16 newFile(lpNewFileName);
  return winrtInternalCopyFileW(existingFile.result(), newFile.result(), bFailIfExists);
}

//-----------------------------------------------------------------------------
BOOL WINAPI winrtCopyFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
  return winrtInternalCopyFileW(lpExistingFileName, lpNewFileName, bFailIfExists);
}

//-----------------------------------------------------------------------------
BOOL WINAPI winrtCopyFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName,
  BOOL    bFailIfExists
)
{
  return winrtInternalCopyFileA(lpExistingFileName, lpNewFileName, bFailIfExists);
}

//-----------------------------------------------------------------------------
static HMODULE WINAPI winrtInternalGetModuleHandleW(
  LPCWSTR lpModuleName
)
{
  return LoadPackagedLibrary(lpModuleName, 0);
}

//-----------------------------------------------------------------------------
static HMODULE WINAPI winrtInternalGetModuleHandleA(
  LPCSTR lpModuleName
)
{
  return winrtInternalGetModuleHandleW(WinRT::StringConvertToUTF16(lpModuleName).result());
}


//-----------------------------------------------------------------------------
HANDLE WINAPI winrtInternalCreateEventW(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCWSTR               lpName
)
{
  return CreateEventEx(lpEventAttributes, lpName, (bManualReset ? CREATE_EVENT_MANUAL_RESET : 0) | (bInitialState ? CREATE_EVENT_INITIAL_SET : 0), EVENT_ALL_ACCESS);
}

//-----------------------------------------------------------------------------
HANDLE WINAPI winrtInternalCreateEventA(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCSTR                lpName
)
{
  return winrtInternalCreateEventW(lpEventAttributes, bManualReset, bInitialState, WinRT::StringConvertToUTF16(lpName).result());
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

//-----------------------------------------------------------------------------
BOOLEAN winrtRtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
)
{
  return winrtInternalRtlGenRandom(RandomBuffer,RandomBufferLength);
}

//-----------------------------------------------------------------------------
int WINAPI winrtMessageBoxW(
  HWND    hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT    uType
)
{
  return winrtInternalMessageBoxW(hWnd, lpText, lpCaption, uType);
}

//-----------------------------------------------------------------------------
int WINAPI winrtMessageBoxA(
  HWND    hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT    uType
)
{
  return winrtInternalMessageBoxA(hWnd,lpText,lpCaption, uType);
}


//-----------------------------------------------------------------------------
HANDLE WINAPI winrtCreateFileW(
  LPCWSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
  return winrtInternalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//-----------------------------------------------------------------------------
HANDLE WINAPI winrtCreateFileA(
  LPCSTR                lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
)
{
  return winrtInternalCreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//-----------------------------------------------------------------------------
DWORD WINAPI winrtGetTempPathW(
  DWORD  nBufferLength,
  LPWSTR lpBuffer
)
{
  return winrtInternalGetTempPathW(nBufferLength, lpBuffer);
}

//-----------------------------------------------------------------------------
DWORD WINAPI winrtGetTempPathA(
  DWORD  nBufferLength,
  LPSTR lpBuffer
)
{
  return winrtInternalGetTempPathA(nBufferLength, lpBuffer);
}

//-----------------------------------------------------------------------------
BOOL WINAPI winrtMoveFileW(
  LPCWSTR lpExistingFileName,
  LPCWSTR lpNewFileName
)
{
  return winrtInternalMoveFileW(lpExistingFileName, lpNewFileName);
}

//-----------------------------------------------------------------------------
BOOL WINAPI winrtMoveFileA(
  LPCSTR lpExistingFileName,
  LPCSTR lpNewFileName
)
{
  return winrtInternalMoveFileA(lpExistingFileName, lpNewFileName);
}

//-----------------------------------------------------------------------------
HMODULE WINAPI winrtGetModuleHandleW(
  LPCWSTR lpModuleName
)
{
  return winrtInternalGetModuleHandleW(lpModuleName);
}

//-----------------------------------------------------------------------------
HMODULE WINAPI winrtGetModuleHandleA(
  LPCSTR lpModuleName
)
{
  return winrtInternalGetModuleHandleA(lpModuleName);
}

//-----------------------------------------------------------------------------
void WINAPI winrtInitializeCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
)
{
  InitializeCriticalSectionEx(lpCriticalSection, 0, 0);
}

//-----------------------------------------------------------------------------
HANDLE WINAPI winrtCreateEventW(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCWSTR               lpName
)
{
  return winrtInternalCreateEventW(lpEventAttributes, bManualReset, bInitialState, lpName);
}

//-----------------------------------------------------------------------------
HANDLE WINAPI winrtCreateEventA(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL                  bManualReset,
  BOOL                  bInitialState,
  LPCSTR               lpName
)
{
  return winrtInternalCreateEventA(lpEventAttributes, bManualReset, bInitialState, lpName);
}


#ifdef __cplusplus
  }
#endif /* __cplusplus */
