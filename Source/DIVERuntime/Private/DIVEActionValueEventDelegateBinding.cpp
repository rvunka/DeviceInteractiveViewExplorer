// Copyright (c) 2026. All Rights Reserved.

#include "DIVEActionValueEventDelegateBinding.h"

#include "DIVEInspectableComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DIVEActionValueEventDelegateBinding)

namespace
{
UDIVEInspectableComponent* FindInspectableOnInstance(UObject* InInstance)
{
	AActor* Actor = Cast<AActor>(InInstance);
	if (!Actor)
	{
		return nullptr;
	}

	return Actor->FindComponentByClass<UDIVEInspectableComponent>();
}

void BindOrUnbind(
	UObject* InInstance,
	const TArray<FDIVEActionValueEventBlueprintBinding>& Bindings,
	const bool bBind)
{
	UDIVEInspectableComponent* Inspectable = FindInspectableOnInstance(InInstance);
	if (!Inspectable || !InInstance)
	{
		return;
	}

	for (const FDIVEActionValueEventBlueprintBinding& Binding : Bindings)
	{
		if (Binding.FunctionNameToBind.IsNone())
		{
			continue;
		}

		if (!InInstance->GetClass()->FindFunctionByName(Binding.FunctionNameToBind))
		{
			continue;
		}

		FScriptDelegate Delegate;
		Delegate.BindUFunction(InInstance, Binding.FunctionNameToBind);
		if (bBind)
		{
			Inspectable->OnActionValueChanged.AddUnique(Delegate);
		}
		else
		{
			Inspectable->OnActionValueChanged.Remove(Delegate);
		}
	}
}
}

void UDIVEActionValueEventDelegateBinding::BindDynamicDelegates(UObject* InInstance) const
{
	BindOrUnbind(InInstance, ActionValueEventBindings, true);
}

void UDIVEActionValueEventDelegateBinding::UnbindDynamicDelegates(UObject* InInstance) const
{
	BindOrUnbind(InInstance, ActionValueEventBindings, false);
}
