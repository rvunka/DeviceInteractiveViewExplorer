// Copyright (c) 2026. All Rights Reserved.

#include "Factories/DIVEUnscrewActionDefinitionFactory.h"

#include "AssetTypeCategories.h"
#include "DIVEUnscrewActionDefinition.h"

UDIVEUnscrewActionDefinitionFactory::UDIVEUnscrewActionDefinitionFactory()
{
	SupportedClass = UDIVEUnscrewActionDefinition::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}

UObject* UDIVEUnscrewActionDefinitionFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn)
{
	(void)Context;
	(void)Warn;
	return NewObject<UDIVEUnscrewActionDefinition>(
		InParent,
		InClass ? InClass : UDIVEUnscrewActionDefinition::StaticClass(),
		InName,
		Flags);
}

FText UDIVEUnscrewActionDefinitionFactory::GetDisplayName() const
{
	return NSLOCTEXT("DIVE", "UnscrewActionDefinitionFactory", "DIVE Unscrew Action Definition");
}

uint32 UDIVEUnscrewActionDefinitionFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::Misc;
}
