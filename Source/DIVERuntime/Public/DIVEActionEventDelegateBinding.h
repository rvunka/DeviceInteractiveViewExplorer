// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEDeviceAction.h"
#include "Engine/DynamicBlueprintBinding.h"

#include "DIVEActionEventDelegateBinding.generated.h"

/** One compiled DIVE Action Event → OnActionExecuted bind entry. */
USTRUCT()
struct DIVERUNTIME_API FDIVEActionEventBlueprintBinding
{
	GENERATED_BODY()

	/** Optional filter class (filtering is also done in the K2 ExpandNode Cast). Stored for diagnostics. */
	UPROPERTY()
	TSubclassOf<UDIVEDeviceAction> ActionClass;

	UPROPERTY()
	FName FunctionNameToBind = NAME_None;
};

/**
 * Runtime binding for EI-like DIVE Action Event nodes.
 * Finds UDIVEInspectableComponent on the actor instance and binds OnActionExecuted.
 */
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
