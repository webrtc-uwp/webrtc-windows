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

BOOLEAN winrtInternalRtlGenRandom(
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

#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

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

BOOLEAN winrtRtlGenRandom(
  PVOID RandomBuffer,
  ULONG RandomBufferLength
)
{
  return winrtInternalRtlGenRandom(RandomBuffer,RandomBufferLength);
}

#ifdef __cplusplus
  }
#endif /* __cplusplus */
