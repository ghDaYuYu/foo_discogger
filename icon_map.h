#pragma once
#include "resource.h"

enum class Icon
{
	Record,
	Quian,
	Quian_Dark
};

using IconMapping = std::unordered_map<Icon, std::map<int, UINT>>;

#define ICON_SIZE_MAPPINGS(BaseResourceId) \
	{ 16, BaseResourceId##_16 }, \
	{ 24, BaseResourceId##_24 }, \
	{ 32, BaseResourceId##_32 }, \
	{ 48, BaseResourceId##_48 }

// clang-format off
const IconMapping ICON_RESOURCE_MAPPINGS_COLOR = {
	{ Icon::Record, { ICON_SIZE_MAPPINGS(IDB_PNG_RECC) } },
	{ Icon::Quian, { ICON_SIZE_MAPPINGS(IDB_PNG_QUIAN) } },
	{ Icon::Quian_Dark, { ICON_SIZE_MAPPINGS(IDB_PNG_QUIAN_DARK) } }
};
// clang-format on

HBITMAP LoadDpiBitmapResource(Icon icon, bool isDark);
HICON LoadDpiIconResource(Icon icon, bool isDark);
