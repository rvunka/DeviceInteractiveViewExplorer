// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "DIVEDeviceActionDefinition.generated.h"

/**
 * Base for pick/context action definitions. DIVE menu creates a Blueprint child so you can Add Variable.
 * Assign Data Asset instances of that Blueprint to catalog rows. Logic stays on IDIVEDeviceActionHandler.
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

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
