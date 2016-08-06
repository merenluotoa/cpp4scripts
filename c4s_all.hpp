/*******************************************************************************
c4s_all.hpp

All inclusive header that can be included into script-like cpp code to provide
easy management of paths and processes.

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

#ifndef CPP4SCRIPTS_ALL_HPP
#define CPP4SCRIPTS_ALL_HPP

#ifdef _MSC_VER
 #define _CRT_SECURE_NO_WARNINGS
#endif

#if defined(__linux) || defined(__APPLE__)
 #include <string.h>
 #include <sys/wait.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <dirent.h>
 #include <pwd.h>
 #include <grp.h>
 #include <stdio.h>
 #include <syslog.h>
 #include <poll.h>
// OSX Only?
 #include <signal.h>
 #ifdef __APPLE__
  #include <mach-o/dyld.h>
 #endif
#endif
#ifdef _MSC_VER
 #include <direct.h>
 #include <io.h>
#endif
#include <sys/stat.h>
#include <time.h>
#include <stdarg.h>
#include <fcntl.h>

#include "cpp4scripts.hpp"

#include "c4s_exception.cpp"
#if defined(__linux) || defined(__APPLE__)
  #include "c4s_user.cpp"
#endif
#include "c4s_path.cpp"
#include "c4s_path_list.cpp"
#include "c4s_variables.cpp"
#include "c4s_process.cpp"
#include "c4s_program_arguments.cpp"
#include "c4s_logger.cpp"
#include "c4s_util.cpp"
#include "c4s_builder.cpp"
#if defined(__linux) || defined(__APPLE__)
 #include "c4s_builder_gcc.cpp"
#else
 #include "c4s_builder_vc.cpp"
#endif
#undef CPP4SCRIPTS_ALL_HPP
#endif
