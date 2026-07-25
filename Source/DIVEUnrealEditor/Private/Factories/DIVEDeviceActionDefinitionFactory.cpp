// Copyright (c) 2026. All Rights Reserved.

#include "Factories/DIVEDeviceActionDefinitionFactory.h"

#include "AssetTypeCategories.h"
#include "DIVEDeviceActionDefinition.h"

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

uint32 UDIVEDeviceActionDefinitionFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}
