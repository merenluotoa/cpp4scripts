/*******************************************************************************
c4s_config.hpp

Defines in this file configure the behaviour of the Cpp4Scripts library. Define
values here, build script or your source code.

--------------------------------------------------------------------------------
This file is part of Cpp4Scripts library.

  Cpp4Scripts is free software: you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation, either version 3 of the License, or (at your option) any
  later version.

  Cpp4Scripts is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details:
  http://www.gnu.org/licenses/lgpl.html


Copyright (c) Menacon Ltd
*******************************************************************************/

#ifndef CPP4SCRIPTS_CONFIG
#define CPP4SCRIPTS_CONFIG

/* If defined, causes c4s_exception-class to be inhereted from STL runtime_error.  Otherwise
c4s_exeption class is not inherited and stands on its own. */
#ifndef C4S_STL_EXCEPTIONS
#define C4S_STL_EXCEPTIONS
#endif

/* Specifies the default number of seconds process-class waits before process termination. Developer can
override the default in the start command. After timeout the library kills the process by
force if it has not stopped by itself by then. Must define an integer value. */
#ifndef C4S_PROC_TIMEOUT
#define C4S_PROC_TIMEOUT 15
#endif

/* Forces path separator to be native to the runtime environment for path-class.  Automatic
conversion is performed at the run-time. E.g. source code has a constant string
"./config/file.xml". If define has been used and the code is run in Windows the path-class
converts the string to: ".\config\file.xml"*/
#ifndef C4S_FORCE_NATIVE_PATH
#define C4S_FORCE_NATIVE_PATH
#endif

/* If log macros are used then log statements can be removed from executable at the compile time.
   The higher the level number, less messages are displayed. 1 = trace ... 7 critical. Set C4S_LOG_LEVEL
   define to 0 to include all logs to build and to 8 to remove all from binary.*/
#ifndef C4S_LOG_LEVEL
 #ifdef _DEBUG
   #define C4S_LOG_LEVEL 2
 #else
   #define C4S_LOG_LEVEL 3
 #endif
#endif

/* Specifies the internal buffer size for logger::vaprt function.*/
#ifndef C4S_LOG_VABUFFER_SIZE
#define C4S_LOG_VABUFFER_SIZE 0x800
#endif

/* Causes (a whole lot of) debug / trace information to appear on stdout. For developer use only!*/
//#ifndef C4S_DEBUGTRACE
//#define C4S_DEBUGTRACE
//#endif

#if defined(__linux) || defined(__APPLE__)
  #include <stdlib.h>
  #include <stddef.h>
  #include <unistd.h>
  #include <errno.h>
  #if defined (STLPORT) && !defined _STLP_USE_UNIX_IO
   #error Unix io is needed in linux build
  #endif
  const int MAX_PROCESS_ARGS = 100;
  #define HANDLE pid_t
  typedef long long int __int64;
  #define C4S_DSEP '/'
  #define C4S_PSEP ':'
  #define C4S_QUOT '\''
  #define SSIZE_T ssize_t
  #define SIZE_T size_t
#endif

#ifdef _WIN32
  #include <windows.h>
  #define C4S_DSEP '\\'
  #define C4S_PSEP ';'
  #define C4S_QUOT '\"'
#endif

const SSIZE_T SSIZE_T_MAX=~0;
const int MAX_LINE=512;
const int MAX_NESTING=50;

// We need the extensions.
#if defined(STLPORT)
 #if defined(_STLP_NO_EXTENSIONS)
   #undef _STLP_NO_EXTENSIONS
 #endif
 #if defined(__STLP_NO_EXTENSIONS)
   #undef __STLP_NO_EXTENSIONS
 #endif
#endif

#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <string>
#include <list>
#ifdef C4S_STL_EXCEPTIONS
  #include <stdexcept>
#endif
#if !defined (STLPORT) || defined(_STLP_USE_NAMESPACES)
using namespace std;
#endif

#endif // CPP4SCRIPTS_CONFIG
