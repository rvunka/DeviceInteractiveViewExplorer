// Copyright (c) 2026. All Rights Reserved.

#include "Factories/DIVEActionCatalogAssetFactory.h"

#include "DIVEActionCatalogAsset.h"
#include "DIVEEditorAssetCategory.h"

UDIVEActionCatalogAssetFactory::UDIVEActionCatalogAssetFactory()
{
	SupportedClass = UDIVEActionCatalogAsset::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDIVEActionCatalogAssetFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	return NewObject<UDIVEActionCatalogAsset>(InParent, InClass, InName, Flags);
}

FText UDIVEActionCatalogAssetFactory::GetDisplayName() const
{
	return NSLOCTEXT("DIVEActionCatalogAssetFactory", "DisplayName", "DIVE Action Catalog");
}

uint32 UDIVEActionCatalogAssetFactory::GetMenuCategories() const
{
	return DIVEEditor::GetDIVEAssetCategory();
}
