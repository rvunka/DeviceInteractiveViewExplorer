// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "AssetTypeCategories.h"
#include "Internationalization/Text.h"

namespace DIVEEditor
{
inline FText GetDIVECategoryText()
{
	return NSLOCTEXT("DIVEEditor", "DIVECategory", "DIVE");
}

EAssetTypeCategories::Type GetDIVEAssetCategory();
void SetDIVEAssetCategory(EAssetTypeCategories::Type Category);
}
