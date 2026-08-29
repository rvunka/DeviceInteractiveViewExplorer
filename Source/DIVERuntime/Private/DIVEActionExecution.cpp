// Copyright (c) 2026. All Rights Reserved.

#include "DIVEActionExecution.h"

#include "DIVEInspectableComponent.h"
#include "DIVELog.h"
#include "Engine/World.h"

namespace DIVEActionExecution
{

bool TryBeginContinuousAction(
	UWorld* World,
	UDIVEInspectableComponent* Inspectable,
	FDIVEContinuousActionSlot& Slot,
	UDIVEContinuousDeviceAction* Action,
	const FDIVEActionContext& Context)
{
	if (!World || !Inspectable || !Action)
	{
		return false;
	}

	// Occupied slot: refuse while a gesture is live. If the action self-completed without End,
	// ActiveAction can linger with IsInteractionActive=false — clean up then proceed.
	if (Slot.ActiveAction.IsValid())
	{
		if (Slot.IsActive())
		{
			return false;
		}
		EndContinuousAction(World, Inspectable, Slot, false);
	}

	FDIVEActionWorldScope WorldScope(Action, World);
	if (!Action->CanExecute(Context))
	{
		UE_LOG(
			LogDIVE,
			Verbose,
			TEXT("Continuous '%s' (%s) CanExecute=false Target=%s Binding=%s"),
			*Action->GetResolvedDisplayName().ToString(),
			*GetNameSafe(Action->GetClass()),
			*GetNameSafe(Context.Target.Get()),
			*Context.BindingId.ToString());
		return false;
	}

	// Mark + value subscribe before Begin so NotifyInteractionValue in Begin has Context + listener.
	Action->MarkInteractionActive(Context);
	Action->OnValueChanged.AddUniqueDynamic(
		Inspectable,
		&UDIVEInspectableComponent::HandleContinuousActionValueChanged);

	if (!Action->BeginInteraction(Context))
	{
		Action->EndInteraction(false);
		Action->OnValueChanged.RemoveDynamic(
			Inspectable,
			&UDIVEInspectableComponent::HandleContinuousActionValueChanged);
		return false;
	}

	Slot.ActiveAction = Action;
	Inspectable->NotifyActionExecuted(Action, Context);
	return true;
}

bool ExecuteResolvedAction(
	UWorld* World,
	UDIVEInspectableComponent* Inspectable,
	FDIVEContinuousActionSlot& Slot,
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context)
{
	if (!World || !Inspectable || !Action)
	{
		return false;
	}

	FDIVEActionWorldScope WorldScope(Action, World);
	if (!Action->CanExecute(Context))
	{
		UE_LOG(
			LogDIVE,
			Verbose,
			TEXT("Action '%s' (%s) CanExecute=false Target=%s Binding=%s"),
			*Action->GetResolvedDisplayName().ToString(),
			*GetNameSafe(Action->GetClass()),
			*GetNameSafe(Context.Target.Get()),
			*Context.BindingId.ToString());
		return false;
	}

	if (UDIVEContinuousDeviceAction* Continuous = Cast<UDIVEContinuousDeviceAction>(Action))
	{
		return TryBeginContinuousAction(World, Inspectable, Slot, Continuous, Context);
	}

	if (!Action->Execute(Context))
	{
		return false;
	}

	Inspectable->NotifyActionExecuted(Action, Context);
	return true;
}

void UpdateContinuousAction(
	UWorld* World,
	FDIVEContinuousActionSlot& Slot,
	const FDIVEInteractionUpdate& Update)
{
	UDIVEContinuousDeviceAction* Continuous = Slot.ActiveAction.Get();
	if (!Continuous)
	{
		Slot.Reset();
		return;
	}

	FDIVEInteractionUpdate Frame = Update;
	if (Frame.DeltaTime <= 0.f)
	{
		Frame.DeltaTime = World ? World->GetDeltaSeconds() : 0.f;
	}

	{
		FDIVEActionWorldScope WorldScope(Continuous, World);
		Continuous->UpdateInteraction(Frame);
	}
}

void EndContinuousAction(
	UWorld* World,
	UDIVEInspectableComponent* Inspectable,
	FDIVEContinuousActionSlot& Slot,
	const bool bCommit)
{
	UDIVEContinuousDeviceAction* Continuous = Slot.ActiveAction.Get();
	if (!Continuous)
	{
		Slot.Reset();
		return;
	}

	{
		FDIVEActionWorldScope WorldScope(Continuous, World);
		// End first so the released value (Press 0, last Rotary tick) still reaches listeners.
		Continuous->EndInteraction(bCommit);
		if (Inspectable)
		{
			Continuous->OnValueChanged.RemoveDynamic(
				Inspectable,
				&UDIVEInspectableComponent::HandleContinuousActionValueChanged);
		}
	}

	Slot.Reset();
}

} // namespace DIVEActionExecution
