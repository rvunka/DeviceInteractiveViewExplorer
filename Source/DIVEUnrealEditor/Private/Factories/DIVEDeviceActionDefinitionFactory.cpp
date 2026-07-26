// Copyright (c) 2026. All Rights Reserved.

#include "Factories/DIVEDeviceActionDefinitionFactory.h"

#include "DIVEDeviceActionDefinition.h"
#include "DIVEEditorAssetCategory.h"
#include "Engine/Blueprint.h"

UDIVEDeviceActionDefinitionFactory::UDIVEDeviceActionDefinitionFactory()
{
	SupportedClass = UBlueprint::StaticClass();
	ParentClass = UDIVEDeviceActionDefinition::StaticClass();
	BlueprintType = BPTYPE_Normal;
	bCreateNew = true;
	bEditAfterNew = true;
	bSkipClassPicker = true;
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
		"Blueprint: add your variables, set ActionId. Then Right-click → Create Data Asset and assign that to the catalog.");
}

uint32 UDIVEDeviceActionDefinitionFactory::GetMenuCategories() const
{
	return DIVEEditor::GetDIVEAssetCategory();
}
