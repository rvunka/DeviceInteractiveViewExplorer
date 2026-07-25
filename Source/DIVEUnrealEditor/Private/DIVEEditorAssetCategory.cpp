// Copyright (c) 2026. All Rights Reserved.

#include "DIVEEditorAssetCategory.h"

#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "DIVEEditorAssets"

namespace DIVEEditor
{
EAssetTypeCategories::Type GetDIVEAssetCategory()
{
	static EAssetTypeCategories::Type Category = EAssetTypeCategories::Misc;
	static bool bRegistered = false;
	if (!bRegistered)
	{
		bRegistered = true;
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		Category = AssetTools.RegisterAdvancedAssetCategory(
			FName(TEXT("DIVE")),
			LOCTEXT("DIVEAssetCategory", "DIVE"));
	}
	return Category;
}
}

#undef LOCTEXT_NAMESPACE
