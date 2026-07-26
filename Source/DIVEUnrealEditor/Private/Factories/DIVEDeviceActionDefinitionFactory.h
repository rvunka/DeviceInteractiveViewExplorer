// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Factories/BlueprintFactory.h"

#include "DIVEDeviceActionDefinitionFactory.generated.h"

/** Creates a Blueprint child of UDIVEDeviceActionDefinition (Add Variable for custom fields). */
UCLASS()
class UDIVEDeviceActionDefinitionFactory : public UBlueprintFactory
{
	GENERATED_BODY()

public:
	UDIVEDeviceActionDefinitionFactory();

	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTip() const override;
	virtual uint32 GetMenuCategories() const override;
};
