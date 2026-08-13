// Copyright (c) 2026. All Rights Reserved.

#include "K2Nodes/K2Node_DIVEActionBoundEvent.h"

#include "DIVEActionEventDelegateBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_DIVEActionBoundEvent)

UK2Node_DIVEActionBoundEvent::UK2Node_DIVEActionBoundEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bInternalEvent = true;
}

UClass* UK2Node_DIVEActionBoundEvent::GetDynamicBindingClass() const
{
	return UDIVEActionEventDelegateBinding::StaticClass();
}

void UK2Node_DIVEActionBoundEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UDIVEActionEventDelegateBinding* ActionBinding =
		CastChecked<UDIVEActionEventDelegateBinding>(BindingObject);

	FDIVEActionEventBlueprintBinding Binding;
	Binding.FunctionNameToBind = CustomFunctionName;
	ActionBinding->ActionEventBindings.Add(Binding);
}
