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

#include "winuwp_compat_std.h"
#include "winuwp_compat_internal.h"

#ifdef WINUWP

#include <Windows.h>

#if !defined(NTDDI_WIN10_RS4) || (WDK_NTDDI_VERSION < NTDDI_WIN10_RS4)

static char *winuwpInternalGetEnvA(const char *envstring)
{
  auto singleton = WinUWP::Environment::singleton();
  return singleton.getEnv(envstring);
}

errno_t winuwpInternalDupEnv(
   char **buffer,  
   size_t *numberOfElements,  
   const char *varname  
)
{
  if ((NULL == buffer) ||
      (NULL == varname)) {
    errno = EINVAL;
    return EINVAL;
  }
  auto singleton = WinUWP::Environment::singleton();
  return singleton.dupEnv(*buffer, numberOfElements, varname);
}

static int winuwpInternalPutEnvA(const char *envstring)
{
  auto singleton = WinUWP::Environment::singleton();
  return singleton.setEnv(envstring);
}

static int winuwpInternalPutEnvW(const wchar_t *envstring)
{
  WinUWP::StringConvertToUTF8 str(envstring);
  return winuwpInternalPutEnvA(str.result());
}

#endif // !defined(NTDDI_WIN10_RS4) || (WDK_NTDDI_VERSION < NTDDI_WIN10_RS4)

#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

#if !defined(NTDDI_WIN10_RS4) || (WDK_NTDDI_VERSION < NTDDI_WIN10_RS4)
char *winuwpGetEnv(const char *varname)
{
  return winuwpInternalGetEnvA(varname);
}

errno_t winuwpDupEnv(
   char **buffer,  
   size_t *numberOfElements,  
   const char *varname  
)
{
  return winuwpInternalDupEnv(buffer, numberOfElements, varname);
}

int winuwpPutEvnA(const char *envstring)
{
  return winuwpInternalPutEnvA(envstring);
}

int winuwpPutEvnW(const wchar_t *envstring)
{
  return winuwpInternalPutEnvW(envstring);
}
#endif // !defined(NTDDI_WIN10_RS4) || (WDK_NTDDI_VERSION < NTDDI_WIN10_RS4)
pid_t winuwpGetPid()
{
  return static_cast<pid_t>(GetCurrentProcessId());
}

#ifdef __cplusplus
  }
#endif /* __cplusplus */

#endif /* WINUWP */

void winuwp_compat_std_noop() {}
