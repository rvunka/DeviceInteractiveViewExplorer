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
 * Shared definition for a custom pick/context action.
 * ActionId / display / toggle are the DIVE contract; put domain params in Params (any USTRUCT).
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

	/** Optional typed parameters (User Defined Struct or C++ USTRUCT). Read in HandleDeviceAction via Definition. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Action", meta = (
		ToolTip = "Pick a struct type and fill fields. No Blueprint subclass required."))
	FInstancedStruct Params;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
