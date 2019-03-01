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

#ifdef WINUWP

#include <Windows.h>

namespace WinUWP
{
  struct StringConvertToUTF8
  {
    StringConvertToUTF8(const wchar_t *str);
    ~StringConvertToUTF8();

    const char *result() const;
    char *result(char *buffer, size_t size) const;
    size_t length() const { return length_; }

  protected:
    char *freeBuffer_{};
    const char *buffer_ {};
    size_t length_ {};
  };

  struct StringConvertToUTF16
  {
    StringConvertToUTF16(const char *str);
    ~StringConvertToUTF16();

    const wchar_t *result() const;
    wchar_t *result(wchar_t *buffer, size_t size) const;
    size_t length() const { return length_; }

  protected:
    wchar_t *freeBuffer_ {};
    const wchar_t *buffer_ {};
    size_t length_ {};
  };

  struct Environment
  {
    Environment();
    ~Environment();

    static Environment &singleton();

    char *getEnv(const char *name);
    errno_t dupEnv(
      char * &outBuffer,
      size_t *outSize,
      const char *varname
    );
    int setEnv(const char *nameValue);

  protected:
    void clearEnv();
    void resizePlusOne();

  protected:
    CRITICAL_SECTION lock_ {};

    struct Info
    {
      Info(
        const char *name,
        const char *value
      );
      ~Info();

      bool get(
        const char *name,
        char * &outResult
      ) const;

    protected:
      char *name_ {};
      char *value_ {};
    };

    Info **infos_ {};
    size_t totalInfos_ {};
  };

}

#endif /* WINUWP */
