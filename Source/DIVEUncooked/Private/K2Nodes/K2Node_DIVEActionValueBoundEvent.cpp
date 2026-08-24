// Copyright (c) 2026. All Rights Reserved.

#include "K2Nodes/K2Node_DIVEActionValueBoundEvent.h"

#include "DIVEActionValueEventDelegateBinding.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_DIVEActionValueBoundEvent)

UK2Node_DIVEActionValueBoundEvent::UK2Node_DIVEActionValueBoundEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bInternalEvent = true;
}

UClass* UK2Node_DIVEActionValueBoundEvent::GetDynamicBindingClass() const
{
	return UDIVEActionValueEventDelegateBinding::StaticClass();
}

void UK2Node_DIVEActionValueBoundEvent::RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const
{
	UDIVEActionValueEventDelegateBinding* ValueBinding =
		CastChecked<UDIVEActionValueEventDelegateBinding>(BindingObject);

	FDIVEActionValueEventBlueprintBinding Binding;
	Binding.FunctionNameToBind = CustomFunctionName;
	ValueBinding->ActionValueEventBindings.Add(Binding);
}
