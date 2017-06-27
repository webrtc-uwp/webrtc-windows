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

#include "winuwp_compat_internal.h"

#ifdef WINUWP

#include <Windows.h>

#include <stdlib.h>
#include <string.h>
#include <wchar.h>

namespace WinUWP
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

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  MainConvertToUTF8::MainConvertToUTF8(Platform::Array<Platform::String^>^ args)
  {
    if (!args) return;

    size_t length = static_cast<decltype(length)>(args->Length);

    argv_ = (char_type **)malloc((length+1)*sizeof(char_type *));
    memset(argv_, 0, (length+1)*sizeof(char_type *));

    for (decltype(length) iter = 0; iter < length; ++iter) {
      auto value = args->get(static_cast<unsigned int>(iter));
      if (!value) continue;
      auto data = value->Data();
      if (!data) continue;

      StringConvertToUTF8 conv(value);
      auto result = conv.result();
      if (!result) continue;

      auto slen = conv.length();
      argv_[iter] = (char_type *)malloc((slen+1)*sizeof(char_type));
      memset(argv_[iter], 0, (slen+1)*sizeof(char_type));

      memcpy(argv_[iter], result, slen);
    }
  }

  //---------------------------------------------------------------------------
  MainConvertToUTF8::~MainConvertToUTF8()
  {
    if (NULL == argv_) return;

    for (int iter = 0; iter < argc_; ++iter) {
      if (NULL == argv_[iter]) continue;
      free(argv_[iter]);
      argv_[iter] = NULL;
    }
    free(argv_);
    argv_ = NULL;
  }


  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  MainConvertToUTF16::MainConvertToUTF16(Platform::Array<Platform::String^>^ args)
  {
    if (!args) return;

    size_t length = static_cast<decltype(length)>(args->Length);

    argv_ = (char_type **)malloc((length+1)*sizeof(char_type *));
    memset(argv_, 0, (length+1)*sizeof(char_type *));

    for (decltype(length) iter = 0; iter < length; ++iter) {
      auto value = args->get(static_cast<unsigned int>(iter));
      if (!value) continue;
      auto data = value->Data();
      if (!data) continue;

      StringConvertToUTF16 conv(value);
      auto result = conv.result();
      if (!result) continue;

      auto slen = conv.length();
      argv_[iter] = (char_type *)malloc((slen+1)*sizeof(char_type));
      memset(argv_[iter], 0, (slen+1)*sizeof(char_type));

      memcpy(argv_[iter], result, slen);
    }
  }

  //---------------------------------------------------------------------------
  MainConvertToUTF16::~MainConvertToUTF16()
  {
    if (NULL == argv_) return;

    for (int iter = 0; iter < argc_; ++iter) {
      if (NULL == argv_[iter]) continue;
      free(argv_[iter]);
      argv_[iter] = NULL;
    }
    free(argv_);
    argv_ = NULL;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  Environment::Environment()
  {
    InitializeCriticalSection(&lock_);
  }

  //---------------------------------------------------------------------------
  Environment::~Environment()
  {
    EnterCriticalSection(&lock_);
    clearEnv();
    LeaveCriticalSection(&lock_);

    DeleteCriticalSection(&lock_);
  }

  //---------------------------------------------------------------------------
  Environment &Environment::singleton()
  {
    static Environment singleton;
    return singleton;
  }

  //---------------------------------------------------------------------------
  char *Environment::getEnv(const char *name)
  {
    char *result = NULL;

    EnterCriticalSection(&lock_);
    if (totalInfos_ < 1) goto done;

    for (int loop = 0; loop < totalInfos_; ++loop) {
      auto info = infos_[loop];
      if (!info) continue;

      if (info->get(name, result)) goto done;
    }

  done:
    {}

    LeaveCriticalSection(&lock_);

    return result;
  }


  //---------------------------------------------------------------------------
  errno_t Environment::dupEnv(
    char * &outBuffer,
    size_t *numberOfElements,
    const char *varname
  )
  {
    errno_t result = 0;

    outBuffer = NULL;
    if (NULL != numberOfElements) numberOfElements = NULL;

    EnterCriticalSection(&lock_);

    auto found = getEnv(varname);
    if (found) {
      auto len = strlen(found);
      outBuffer = (char *)malloc(sizeof(char)*(len + 1));
      if (outBuffer) {
        memset(outBuffer, 0, sizeof(char)*(len + 1));
        memcpy(outBuffer, found, sizeof(char)*(len + 1));
        if (numberOfElements) *numberOfElements = len;
      } else {
        result = ENOMEM;
        errno = result;
      }
    }

    LeaveCriticalSection(&lock_);

    return result;
  }

  //---------------------------------------------------------------------------
  int Environment::setEnv(const char *nameValue)
  {
    if (NULL == nameValue) return -1;

    auto len = strlen(nameValue);
    char *nameValuePair = (char *)malloc(sizeof(char)*(len+1));
    memset(nameValuePair, 0, sizeof(char)*(len+1));
    memcpy(nameValuePair, nameValue, sizeof(char)*len);

    char *value = nameValuePair;
    while ('\0' != *value) {
      if ('=' == *value) {
        *value = '\0';
        ++value;
        break;
      }
      ++value;
    }
    if ('\0' == *value) value = NULL;

    auto info = new Info(nameValuePair, value);

    EnterCriticalSection(&lock_);

    resizePlusOne();
    infos_[0] = info;

    LeaveCriticalSection(&lock_);

    free(nameValuePair);
    nameValuePair = NULL;

    return 0;
  }

  //---------------------------------------------------------------------------
  void Environment::clearEnv()
  {
    if (NULL == infos_) return;

    for (auto loop = 0; loop < totalInfos_; ++loop) {
      if (NULL == infos_[loop]) continue;

      delete infos_[loop];
      infos_[loop] = NULL;
    }

    delete [] infos_;
    infos_ = NULL;
  }

  //---------------------------------------------------------------------------
  void Environment::resizePlusOne()
  {
    typedef Info * InfoPtr;
    if (NULL == infos_) {
      totalInfos_ = 1;
      infos_ = new InfoPtr[totalInfos_];
      infos_[0] = NULL;
      return;
    }

    size_t oldTotalInfos = totalInfos_;
    auto oldInfos = infos_;

    ++totalInfos_;
    infos_ = new InfoPtr[totalInfos_];
    infos_[0] = NULL;

    Info **sourcePos = oldInfos;
    Info **destPos = infos_;

    for (auto loop = 0; loop < oldTotalInfos; ++loop) {
      destPos[loop+1] = sourcePos[loop];
    }

    delete [] oldInfos;
    oldInfos = NULL;
  }

  //---------------------------------------------------------------------------
  Environment::Info::Info(
     const char *name,
     const char *value
  )
  {
    if (NULL != name) {
      size_t len = strlen(name);
      name_ = (char *)malloc(sizeof(char)*(len+1));
      memset(name_,0,sizeof(char)*(len+1));
      memcpy(name_, name,sizeof(char)*(len+1));
    }
    if (NULL != value) {
      size_t len = strlen(value);
      value_ = (char *)malloc(sizeof(char)*(len+1));
      memset(value_,0,sizeof(char)*(len+1));
      memcpy(value_, value,sizeof(char)*(len+1));
    }
  }

  //---------------------------------------------------------------------------
  Environment::Info::~Info()
  {
    if (NULL != name_) {
      free(name_);
      name_ = NULL;
    }
    if (NULL != value_) {
      free(value_);
      value_ = NULL;
    }
  }

  //---------------------------------------------------------------------------
  bool Environment::Info::get(
    const char *name,
    char * &outResult    
  ) const
  {
    if (NULL == name_) return false;

    if (0 != _stricmp(name, name_)) return false;

    outResult = value_;

    return true;
  }

} /* namespace WinUWP */

#endif /* WINUWP */

void winuwp_compat_internal_noop() {}
