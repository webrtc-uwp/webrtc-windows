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

#include <Windows.h>
#include <stdlib.h>

static char *winrtInternalGetCwd(char *buf, size_t size)
{
  auto current = Windows::Storage::ApplicationData::Current;
  if (!current) return (char *)NULL;

  auto localFolder = current->LocalFolder;
  if (!localFolder) return (char *)NULL;

  auto folder = localFolder->Path;
  if (!folder) return (char *)NULL;

  auto len8 = WideCharToMultiByte(CP_UTF8, 0, folder->Data(), folder->Length(),
    NULL, 0, NULL, NULL);

  if (len8 < 1) return (char *)NULL;
  if (static_cast<int>(size) < len8+1) return (char *)NULL;
  if (NULL == buf) buf = (char *)malloc((len8+1)*sizeof(char)); // callee must free

  memset(buf, 0, (len8+1)*sizeof(char));

  auto result = WideCharToMultiByte(CP_UTF8, 0, folder->Data(), folder->Length(),
    (LPSTR)buf, len8, NULL, NULL);

  if (0 == result) return (char *)NULL;

  return buf;
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
