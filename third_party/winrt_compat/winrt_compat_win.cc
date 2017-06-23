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

namespace WinRT
{
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  StringConvertToUTF8::StringConvertToUTF8(const wchar_t *str)
  {
    if (!str) return;

    auto count = wcslen(str);
    auto len8 = WideCharToMultiByte(CP_UTF8, 0, str, static_cast<int>(count), NULL, 0, NULL, NULL);

    if (len8 < 1) return;
    freeBuffer_ = (char *)malloc((len8 + 1) * sizeof(char)); // callee must free
    memset(freeBuffer_, 0, (len8 + 1) * sizeof(char));

    auto result = WideCharToMultiByte(CP_UTF8, 0, str, static_cast<int>(count), freeBuffer_, len8, NULL, NULL);
    if (0 == result) return;
    length_ = static_cast<decltype(length_)>(len8);
    buffer_ = freeBuffer_;
  }

  //---------------------------------------------------------------------------
  StringConvertToUTF8::StringConvertToUTF8(Platform::String ^str)
  {
    if (!str) return;

    auto count = str->Length();
    auto len8 = WideCharToMultiByte(CP_UTF8, 0, str->Data(), count, NULL, 0, NULL, NULL);

    if (len8 < 1) return;
    freeBuffer_ = (char *)malloc((len8 + 1) * sizeof(char)); // callee must free
    memset(freeBuffer_, 0, (len8 + 1) * sizeof(char));

    auto result = WideCharToMultiByte(CP_UTF8, 0, str->Data(), count, freeBuffer_, len8, NULL, NULL);
    if (0 == result) return;
    length_ = static_cast<decltype(length_)>(len8);
    buffer_ = freeBuffer_;
  }

  //---------------------------------------------------------------------------
  StringConvertToUTF8::~StringConvertToUTF8()
  {
    if (NULL == freeBuffer_) return;
    free(freeBuffer_);
    freeBuffer_ = NULL;
  }

  //---------------------------------------------------------------------------
  const char *StringConvertToUTF8::result() const
  {
    return buffer_;
  }

  //---------------------------------------------------------------------------
  char *StringConvertToUTF8::result(char *buffer, size_t size) const
  {
    if (!buffer_) return NULL;

    if (NULL == buffer) {
      buffer = (char *)malloc((length_ + 1) * sizeof(char));
      memset(buffer, 0, (length_ + 1) * sizeof(char));
      size = length_;
    }

    if (size < length_) return NULL;

    memcpy(buffer, buffer_, length_ * sizeof(char));
    if (length_ < size) {
      buffer[length_] = '\0';
    }
    return buffer;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  StringConvertToUTF16::StringConvertToUTF16(const char *str)
  {
    if (!str) return;

    auto count = strlen(str);
    auto len16 = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(count), NULL, 0);

    if (len16 < 1) return;
    freeBuffer_ = (wchar_t *)malloc((len16 + 1) * sizeof(wchar_t)); // callee must free
    memset(freeBuffer_, 0, (len16 + 1) * sizeof(wchar_t));

    auto result = MultiByteToWideChar(CP_UTF8, 0, str, static_cast<int>(count), freeBuffer_, len16);
    if (0 == result) return;
    length_ = static_cast<decltype(length_)>(len16);
    buffer_ = freeBuffer_;
  }

  //---------------------------------------------------------------------------
  StringConvertToUTF16::StringConvertToUTF16(Platform::String ^str)
  {
    if (!str) return;
    if (!str->Data()) return;

    auto count = str->Length();
    freeBuffer_ = (wchar_t *)malloc((count + 1) * sizeof(wchar_t));
    memset(freeBuffer_, 0, (count + 1) * sizeof(wchar_t));

    memcpy(freeBuffer_, str->Data(), count * sizeof(wchar_t));
    length_ = static_cast<decltype(length_)>(count);
    buffer_ = freeBuffer_;
  }

  //---------------------------------------------------------------------------
  StringConvertToUTF16::~StringConvertToUTF16()
  {
  }

  //---------------------------------------------------------------------------
  const wchar_t *StringConvertToUTF16::result() const
  {
    return buffer_;
  }

  //---------------------------------------------------------------------------
  wchar_t *StringConvertToUTF16::result(wchar_t *buffer, size_t size) const
  {
    if (!buffer_) return NULL;

    if (NULL == buffer) {
      buffer = (wchar_t *)malloc((length_ + 1) * sizeof(wchar_t));
      memset(buffer, 0, (length_ + 1) * sizeof(wchar_t));
      size = length_;
    }

    if (size < length_) return NULL;

    memcpy(buffer, buffer_, length_ * sizeof(wchar_t));
    if (length_ < size) {
      buffer[length_] = static_cast<wchar_t>(0);
    }
    return buffer;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  StringConvertToPlatformString::StringConvertToPlatformString(const char *str)
  {
    if (NULL == str) return;

    StringConvertToUTF16 wstr(str);
    auto result = wstr.result();
    if (!result) return;

    buffer_ = ref new Platform::String(result);
  }

  //---------------------------------------------------------------------------
  StringConvertToPlatformString::StringConvertToPlatformString(const wchar_t *str)
  {
    if (NULL == str) return;
    buffer_ = ref new Platform::String(str);
  }

  //---------------------------------------------------------------------------
  StringConvertToPlatformString::~StringConvertToPlatformString()
  {
  }

  //---------------------------------------------------------------------------
  Platform::String ^StringConvertToPlatformString::result() const
  {
    return buffer_;
  }

}

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

#ifdef __cplusplus
  }
#endif /* __cplusplus */
