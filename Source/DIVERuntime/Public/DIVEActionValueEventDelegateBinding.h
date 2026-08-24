// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DynamicBlueprintBinding.h"

#include "DIVEActionValueEventDelegateBinding.generated.h"

USTRUCT()
struct DIVERUNTIME_API FDIVEActionValueEventBlueprintBinding
{
	GENERATED_BODY()

	UPROPERTY()
	FName FunctionNameToBind = NAME_None;
};

/** Binds DIVE Action Value Event nodes to Inspectable.OnActionValueChanged on the actor instance. */
UCLASS()
class DIVERUNTIME_API UDIVEActionValueEventDelegateBinding : public UDynamicBlueprintBinding
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FDIVEActionValueEventBlueprintBinding> ActionValueEventBindings;

	virtual void BindDynamicDelegates(UObject* InInstance) const override;
	virtual void UnbindDynamicDelegates(UObject* InInstance) const override;
};
