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
		Absolutes.Add(Value.Absolute);
		AbsoluteMaxes.Add(Value.AbsoluteMax);
		Units.Add(Value.Unit);
	}

	UFUNCTION()
	void HandleExecuted(UDIVEDeviceAction* Action, const FDIVEActionContext& Context)
	{
		(void)Action;
		(void)Context;
		++ExecutedCount;
	}

	int32 ExecutedCount = 0;
	TArray<float> Absolutes;
	TArray<float> AbsoluteMaxes;
	TArray<EDIVEInteractionValueUnit> Units;
};

UCLASS()
class UDIVETestNeverCondition : public UDIVEActionCondition
{
	GENERATED_BODY()

public:
	virtual bool Evaluate_Implementation(const FDIVEActionContext& Context) const override
	{
		(void)Context;
		return false;
	}
};
