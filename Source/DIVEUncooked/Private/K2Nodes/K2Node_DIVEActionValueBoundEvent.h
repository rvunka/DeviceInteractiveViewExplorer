// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "K2Node_Event.h"

#include "K2Node_DIVEActionValueBoundEvent.generated.h"

class UDynamicBlueprintBinding;

/** Intermediate event bound to Inspectable.OnActionValueChanged (created by ExpandNode). */
UCLASS()
class UK2Node_DIVEActionValueBoundEvent : public UK2Node_Event
{
	GENERATED_BODY()

public:
	UK2Node_DIVEActionValueBoundEvent(const FObjectInitializer& ObjectInitializer);

	virtual UClass* GetDynamicBindingClass() const override;
	virtual void RegisterDynamicBinding(UDynamicBlueprintBinding* BindingObject) const override;
};
