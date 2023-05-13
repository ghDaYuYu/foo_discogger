#pragma once
#include "../helpers/foobar2000+atl.h"

#pragma warning(push, 1)
#pragma warning(disable : 4068)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"

#ifndef WINVER
#define WINVER 0x601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x601
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x601
#endif

#include <winsdkver.h>

#include "../helpers/foobar2000+atl.h"

#include <windows.h>
#include <string>
#include <exception>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <memory>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctime>
#include <WinUser.h>

#include <CommCtrl.h>
#include <commoncontrols.h>
#include <atlframe.h>
#include <atlctrlx.h>
#include <atlctrls.h>

#include "SDK/foobar2000.h"
#include "SDK/filesystem_helper.h"
#include "SDK/album_art.h"

#include "helpers/helpers.h"
#include "helpers/atl-misc.h"
#include "helpers/WindowPositionUtils.h"

#include "libPPUI/win32_op.h"

#include "libPPUI/win32_utility.h"

#include "libPPUI/CListControl-Cells.h"
#include "libPPUI/InPlaceEdit.h"
#include "libPPUI/InPlaceEditTable.h"
#include "liboauthcpp/liboauthcpp.h"

#include "version.h"

#include "wtl_helpers.h"
