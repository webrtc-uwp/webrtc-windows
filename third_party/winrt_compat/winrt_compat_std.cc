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

#include "winrt_compat_std.h"
#include "winrt_compat_internal.h"

#ifdef WINRT

#include <Windows.h>

static char *winrtInternalGetCwd(char *buf, size_t size)
{
  auto current = Windows::Storage::ApplicationData::Current;
  if (!current) return (char *)NULL;

  auto localFolder = current->LocalFolder;
  if (!localFolder) return (char *)NULL;

  auto folder = localFolder->Path;
  if (!folder) return (char *)NULL;

  WinRT::StringConvertToUTF8 str(folder);
  return str.result(buf, size);
}

#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

char *winrtGetCwd(char *buf, size_t size)
{
  return winrtInternalGetCwd(buf, size);
}

char *winrtGetEnv(
   const char *varname   
)
{
  return NULL;
}

#ifdef __cplusplus
  }
#endif /* __cplusplus */

#endif /* WINRT */

void winrt_compat_std_noop() {}
