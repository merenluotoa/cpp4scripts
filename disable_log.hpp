/*******************************************************************************
disable_log.h
Including this file will disable the loggind entries completely from the source
code at compile time.
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

Copyright (c) Menacon Ltd, Finland
*******************************************************************************/
#define CS_PRINT_TRCE(x)
#define CS_PRINT_DEBU(x)
#define CS_PRINT_INFO(x)
#define CS_PRINT_NOTE(x)
#define CS_PRINT_WARN(x)
#define CS_PRINT_ERRO(x)
#define CS_PRINT_CRIT(x)
#define CS_VAPRT_TRCE(x, ...)
#define CS_VAPRT_DEBU(x, ...)
#define CS_VAPRT_INFO(x, ...)
#define CS_VAPRT_NOTE(x, ...)
#define CS_VAPRT_WARN(x, ...)
#define CS_VAPRT_ERRO(x, ...)
#define CS_VAPRT_CRIT(x, ...)

