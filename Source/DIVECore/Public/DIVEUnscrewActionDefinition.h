// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceActionDefinition.h"

#include "DIVEUnscrewActionDefinition.generated.h"

/** Unscrew-style action: default turn count; catalog row may override. */
UCLASS(BlueprintType, Blueprintable)
class DIVECORE_API UDIVEUnscrewActionDefinition : public UDIVEDeviceActionDefinition
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Unscrew", meta = (ClampMin = "1"))
	int32 DefaultTurnCount = 3;
};
