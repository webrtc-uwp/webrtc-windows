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

namespace WinRT
{
  struct StringConvertToUTF8
  {
    StringConvertToUTF8(const wchar_t *str);
    StringConvertToUTF8(Platform::String ^str);
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
    StringConvertToUTF16(Platform::String ^str);
    ~StringConvertToUTF16();

    const wchar_t *result() const;
    wchar_t *result(wchar_t *buffer, size_t size) const;
    size_t length() const { return length_; }

  protected:
    wchar_t *freeBuffer_ {};
    const wchar_t *buffer_ {};
    size_t length_ {};
  };

  struct StringConvertToPlatformString
  {
    StringConvertToPlatformString(const char *str);
    StringConvertToPlatformString(const wchar_t *str);
    ~StringConvertToPlatformString();

    Platform::String ^result() const;

  protected:
    Platform::String ^buffer_;
  };

  struct MainConvertToUTF8
  {
    typedef char char_type;
    MainConvertToUTF8(Platform::Array<Platform::String^>^ args);
    ~MainConvertToUTF8();

    int argc() const { return argc_; }
    char_type **argv() const { return argv_; }

  protected:
    int argc_ {};
    char_type **argv_ {};
  };

  struct MainConvertToUTF16
  {
    typedef char char_type;
    MainConvertToUTF16(Platform::Array<Platform::String^>^ args);
    ~MainConvertToUTF16();

    int argc() const { return argc_; }
    char_type **argv() const { return argv_; }

  protected:
    int argc_ {};
    char_type **argv_ {};
  };

}
