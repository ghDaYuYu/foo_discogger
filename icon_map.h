#pragma once
#include "resource.h"

enum class Icon
{
	Record	
};

using IconMapping = std::unordered_map<Icon, std::map<int, UINT>>;

#define ICON_SIZE_MAPPINGS(BaseResourceId) \
	{ 16, BaseResourceId##_16 }, \
	{ 24, BaseResourceId##_24 }, \
	{ 32, BaseResourceId##_32 }, \
	{ 48, BaseResourceId##_48 }

// clang-format off
const IconMapping ICON_RESOURCE_MAPPINGS_COLOR = {
	{ Icon::Record, { ICON_SIZE_MAPPINGS(IDB_PNG_RECC) } }
};
// clang-format on

HBITMAP LoadDpiBitmapResource(Icon icon);