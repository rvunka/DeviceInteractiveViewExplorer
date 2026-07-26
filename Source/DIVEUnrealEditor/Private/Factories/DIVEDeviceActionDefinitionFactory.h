// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"

#include "DIVEDeviceActionDefinitionFactory.generated.h"

/** Creates a thin UDIVEDeviceActionDefinition DataAsset (ActionId + menu defaults). */
UCLASS()
class UDIVEDeviceActionDefinitionFactory : public UFactory
{
	GENERATED_BODY()

public:
	UDIVEDeviceActionDefinitionFactory();

	virtual UObject* FactoryCreateNew(
		UClass* InClass,
		UObject* InParent,
		FName InName,
		EObjectFlags Flags,
		UObject* Context,
		FFeedbackContext* Warn) override;
	virtual bool ShouldShowInNewMenu() const override { return true; }
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTip() const override;
	virtual uint32 GetMenuCategories() const override;
};
