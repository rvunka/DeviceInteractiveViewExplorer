// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "AssetTypeActions_Base.h"
#include "DIVEActionCatalogAsset.h"
#include "DIVEEditorAssetCategory.h"

class FAssetTypeActions_DIVEActionCatalog : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override
	{
		return NSLOCTEXT("AssetTypeActions_DIVEActionCatalog", "Name", "DIVE Action Catalog");
	}

	virtual FColor GetTypeColor() const override
	{
		return FColor(72, 152, 196);
	}

	virtual UClass* GetSupportedClass() const override
	{
		return UDIVEActionCatalogAsset::StaticClass();
	}

	virtual uint32 GetCategories() override
	{
		return DIVEEditor::GetDIVEAssetCategory();
	}
};
