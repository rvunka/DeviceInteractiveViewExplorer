// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceAction.h"

#include "DIVEActionValueSinkTestTypes.generated.h"

UCLASS()
class UDIVETestActionValueSink : public UObject
{
	GENERATED_BODY()

public:
	TArray<float> Normalized;

	UFUNCTION()
	void HandleValue(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		const FDIVEInteractionValue& Value)
	{
		(void)Action;
		(void)Context;
		Normalized.Add(Value.Normalized);
	}
};
