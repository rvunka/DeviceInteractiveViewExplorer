// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DynamicBlueprintBinding.h"

#include "DIVEActionEventDelegateBinding.generated.h"

USTRUCT()
struct DIVERUNTIME_API FDIVEActionEventBlueprintBinding
{
	GENERATED_BODY()

	UPROPERTY()
	FName FunctionNameToBind = NAME_None;
};

/** Binds DIVE Action Event nodes to Inspectable.OnActionExecuted on the actor instance. */
UCLASS()
class DIVERUNTIME_API UDIVEActionEventDelegateBinding : public UDynamicBlueprintBinding
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FDIVEActionEventBlueprintBinding> ActionEventBindings;

	virtual void BindDynamicDelegates(UObject* InInstance) const override;
	virtual void UnbindDynamicDelegates(UObject* InInstance) const override;
};
