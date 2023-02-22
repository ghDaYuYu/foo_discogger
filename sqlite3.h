#pragma once

#ifndef FBSQLITE3_DLL
#include "sqlite3ren.h"
#else
#define SQLITE_API __declspec(dllimport)
#endif
#include "..\discogger libs\sqlite-amal-3401\include\sqlite3.h"
