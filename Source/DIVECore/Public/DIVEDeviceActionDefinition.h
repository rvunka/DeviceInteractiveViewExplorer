// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "DIVEDeviceActionDefinition.generated.h"

/**
 * Thin shared definition for a custom pick/context action.
 * DIVE reads ActionId / display / toggle. Domain data goes in Settings (your USTRUCT / User Defined Struct).
 * Logic stays on IDIVEDeviceActionHandler.
 */
UCLASS(BlueprintType, Blueprintable)
class DIVECORE_API UDIVEDeviceActionDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Action", meta = (
		ToolTip = "Stable dispatch id (e.g. Unscrew, Toggle). Used by IDIVEDeviceActionHandler."))
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Action", meta = (
		ToolTip = "Menu label when the catalog row DisplayName is empty."))
	FText DefaultDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Action", meta = (
		ToolTip = "When true, DIVE treats the row as a toggle (Is_* suffix / flip after handler)."))
	bool bToggleActiveSuffix = false;

	/** Optional domain settings: create a User Defined Struct, pick it here, fill fields. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Action", meta = (
		ToolTip = "Your struct type + values (e.g. UnscrewParams). Leave empty when ActionId alone is enough."))
	FInstancedStruct Settings;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
