// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Factories/BlueprintFactory.h"

#include "DIVEDeviceActionBlueprintFactory.generated.h"

UCLASS()
class UDIVEDeviceActionBlueprintFactory : public UBlueprintFactory
{
	GENERATED_BODY()

public:
	UDIVEDeviceActionBlueprintFactory();

	virtual bool ConfigureProperties() override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTip() const override;
	virtual uint32 GetMenuCategories() const override;
	virtual FString GetDefaultNewAssetName() const override;
};

UCLASS()
class UDIVEContinuousDeviceActionBlueprintFactory : public UBlueprintFactory
{
	GENERATED_BODY()

public:
	UDIVEContinuousDeviceActionBlueprintFactory();

	virtual bool ConfigureProperties() override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTip() const override;
	virtual uint32 GetMenuCategories() const override;
	virtual FString GetDefaultNewAssetName() const override;
};

UCLASS()
class UDIVEActionConditionBlueprintFactory : public UBlueprintFactory
{
	GENERATED_BODY()

public:
	UDIVEActionConditionBlueprintFactory();

	virtual bool ConfigureProperties() override;
	virtual FText GetDisplayName() const override;
	virtual FText GetToolTip() const override;
	virtual uint32 GetMenuCategories() const override;
	virtual FString GetDefaultNewAssetName() const override;
};
