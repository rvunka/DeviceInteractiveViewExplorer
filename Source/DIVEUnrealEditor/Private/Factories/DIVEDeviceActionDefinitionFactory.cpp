// Copyright (c) 2026. All Rights Reserved.

#include "Factories/DIVEDeviceActionDefinitionFactory.h"

#include "DIVEDeviceActionDefinition.h"
#include "DIVEEditorAssetCategory.h"

UDIVEDeviceActionDefinitionFactory::UDIVEDeviceActionDefinitionFactory()
{
	SupportedClass = UDIVEDeviceActionDefinition::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDIVEDeviceActionDefinitionFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	(void)Context;
	(void)Warn;
	return NewObject<UDIVEDeviceActionDefinition>(
		InParent,
		InClass ? InClass : UDIVEDeviceActionDefinition::StaticClass(),
		InName,
		Flags);
}

FText UDIVEDeviceActionDefinitionFactory::GetDisplayName() const
{
	return NSLOCTEXT("DIVE", "DeviceActionDefinitionFactory", "DIVE Device Action Definition");
}

FText UDIVEDeviceActionDefinitionFactory::GetToolTip() const
{
	return NSLOCTEXT(
		"DIVE",
		"DeviceActionDefinitionFactoryTip",
		"DataAsset: set ActionId (+ display/toggle). Optional Settings = your User Defined Struct.");
}

uint32 UDIVEDeviceActionDefinitionFactory::GetMenuCategories() const
{
	return DIVEEditor::GetDIVEAssetCategory();
}
