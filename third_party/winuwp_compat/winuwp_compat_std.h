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

#include <stddef.h>

#ifdef __cplusplus
namespace webrtc
{
  namespace TickTime
  {
  	inline void DisableFakeClock() {}
  }
}
#endif /* __cplusplus */

#ifdef __cplusplus
  extern "C" {
#endif /* __cplusplus */

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
char *winuwpGetEnv(
   const char *varname   
);

inline char *getenv(const char *varname)
{
	return winuwpGetEnv(varname);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

errno_t winuwpDupEnv(
   char **buffer,  
   size_t *numberOfElements,  
   const char *varname  
);  

inline errno_t _dupenv_s(  
   char **buffer,  
   size_t *numberOfElements,  
   const char *varname  
)
{
  return winuwpDupEnv(buffer, numberOfElements, varname);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
int winuwpPutEvnA(const char *envstring);
int winuwpPutEvnW(const wchar_t *envstring);

inline int _putenv(const char *envstring)
{
  return winuwpPutEvnA(envstring);
}

inline int _wputenv(const wchar_t *envstring)
{
  return winuwpPutEvnW(envstring);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
char *winuwpGetCwd(char *buf, size_t size);

inline char *getcwd(
	char *buf,
	size_t size
)
{
	return winuwpGetCwd(buf, size);
}

/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
typedef int pid_t;

pid_t winuwpGetPid();

inline pid_t getpid(void)
{
  return winuwpGetPid();
}

inline int _getpid(void)
{
  return winuwpGetPid();
}

#ifdef __cplusplus
  }
#endif /* __cplusplus */

#endif /* WINUWP */
