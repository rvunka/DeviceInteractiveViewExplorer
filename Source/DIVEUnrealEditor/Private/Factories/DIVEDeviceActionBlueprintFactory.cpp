// Copyright (c) 2026. All Rights Reserved.

#include "Factories/DIVEDeviceActionBlueprintFactory.h"

#include "DIVEDeviceAction.h"
#include "DIVEEditorAssetCategory.h"
#include "Engine/Blueprint.h"

UDIVEDeviceActionBlueprintFactory::UDIVEDeviceActionBlueprintFactory()
{
	SupportedClass = UBlueprint::StaticClass();
	ParentClass = UDIVEDeviceAction::StaticClass();
	BlueprintType = BPTYPE_Normal;
	bSkipClassPicker = true;
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UDIVEDeviceActionBlueprintFactory::ConfigureProperties()
{
	return true;
}

FText UDIVEDeviceActionBlueprintFactory::GetDisplayName() const
{
	return NSLOCTEXT("DIVEDeviceActionBlueprintFactory", "DisplayName", "DIVE Device Action");
}

FText UDIVEDeviceActionBlueprintFactory::GetToolTip() const
{
	return NSLOCTEXT(
		"DIVEDeviceActionBlueprintFactory",
		"Tooltip",
		"Blueprint subclass of DIVE Device Action. Override Execute for click handlers.");
}

uint32 UDIVEDeviceActionBlueprintFactory::GetMenuCategories() const
{
	return DIVEEditor::GetDIVEAssetCategory();
}

FString UDIVEDeviceActionBlueprintFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("BP_DIVE_Action"));
}

UDIVEContinuousDeviceActionBlueprintFactory::UDIVEContinuousDeviceActionBlueprintFactory()
{
	SupportedClass = UBlueprint::StaticClass();
	ParentClass = UDIVEContinuousDeviceAction::StaticClass();
	BlueprintType = BPTYPE_Normal;
	bSkipClassPicker = true;
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UDIVEContinuousDeviceActionBlueprintFactory::ConfigureProperties()
{
	return true;
}

FText UDIVEContinuousDeviceActionBlueprintFactory::GetDisplayName() const
{
	return NSLOCTEXT("DIVEContinuousDeviceActionBlueprintFactory", "DisplayName", "DIVE Continuous Device Action");
}

FText UDIVEContinuousDeviceActionBlueprintFactory::GetToolTip() const
{
	return NSLOCTEXT(
		"DIVEContinuousDeviceActionBlueprintFactory",
		"Tooltip",
		"Blueprint subclass for hold/drag actions. Override BeginInteraction / UpdateInteraction(FDIVEInteractionUpdate) / EndInteraction.");
}

uint32 UDIVEContinuousDeviceActionBlueprintFactory::GetMenuCategories() const
{
	return DIVEEditor::GetDIVEAssetCategory();
}

FString UDIVEContinuousDeviceActionBlueprintFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("BP_DIVE_ContinuousAction"));
}

UDIVEActionConditionBlueprintFactory::UDIVEActionConditionBlueprintFactory()
{
	SupportedClass = UBlueprint::StaticClass();
	ParentClass = UDIVEActionCondition::StaticClass();
	BlueprintType = BPTYPE_Normal;
	bSkipClassPicker = true;
	bCreateNew = true;
	bEditAfterNew = true;
}

bool UDIVEActionConditionBlueprintFactory::ConfigureProperties()
{
	return true;
}

FText UDIVEActionConditionBlueprintFactory::GetDisplayName() const
{
	return NSLOCTEXT("DIVEActionConditionBlueprintFactory", "DisplayName", "DIVE Action Condition");
}

FText UDIVEActionConditionBlueprintFactory::GetToolTip() const
{
	return NSLOCTEXT(
		"DIVEActionConditionBlueprintFactory",
		"Tooltip",
		"Blueprint predicate for action menu visibility. Override Evaluate (e.g. hide Remove Cover until bolts are free).");
}

uint32 UDIVEActionConditionBlueprintFactory::GetMenuCategories() const
{
	return DIVEEditor::GetDIVEAssetCategory();
}

FString UDIVEActionConditionBlueprintFactory::GetDefaultNewAssetName() const
{
	return FString(TEXT("BP_DIVE_Condition"));
}
