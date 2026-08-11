// Copyright (c) 2026. All Rights Reserved.

#include "DIVEActionEventDelegateBinding.h"

#include "DIVEInspectableComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(DIVEActionEventDelegateBinding)

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
	const TArray<FDIVEActionEventBlueprintBinding>& Bindings,
	bool bBind)
{
	UDIVEInspectableComponent* Inspectable = FindInspectableOnInstance(InInstance);
	if (!Inspectable || !InInstance)
	{
		return;
	}

	for (const FDIVEActionEventBlueprintBinding& Binding : Bindings)
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
			Inspectable->OnActionExecuted.AddUnique(Delegate);
		}
		else
		{
			Inspectable->OnActionExecuted.Remove(Delegate);
		}
	}
}
}

void UDIVEActionEventDelegateBinding::BindDynamicDelegates(UObject* InInstance) const
{
	BindOrUnbind(InInstance, ActionEventBindings, true);
}

void UDIVEActionEventDelegateBinding::UnbindDynamicDelegates(UObject* InInstance) const
{
	BindOrUnbind(InInstance, ActionEventBindings, false);
}
