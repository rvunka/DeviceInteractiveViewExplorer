// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceAction.h"

#include "Engine/World.h"

bool UDIVEActionCondition::Evaluate_Implementation(const FDIVEActionContext& Context) const
{
	(void)Context;
	return true;
}

UWorld* UDIVEDeviceAction::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	// Prefer the execution-time world injected by the runtime. This makes catalog-hosted actions
	// (whose Outer chain leads to an asset package, not a world) behave identically to
	// component-hosted actions for all Blueprint world-context nodes.
	if (UWorld* Injected = ExecutionWorld.Get())
	{
		return Injected;
	}

	for (const UObject* Outer = GetOuter(); Outer; Outer = Outer->GetOuter())
	{
		if (UWorld* World = Outer->GetWorld())
		{
			return World;
		}
	}

	return nullptr;
}

bool UDIVEDeviceAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (Condition && !Condition->Evaluate(Context))
	{
		return false;
	}

	return true;
}

FDIVEActionDisplayState UDIVEDeviceAction::GetDisplayState_Implementation(const FDIVEActionContext& Context) const
{
	FDIVEActionDisplayState State;
	State.DisplayName = GetResolvedDisplayName();
	State.bEnabled = CanExecute(Context);
	State.bVisible = !Condition || Condition->Evaluate(Context);
	State.bChecked = false;
	return State;
}

bool UDIVEDeviceAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	(void)Context;
	return false;
}

FText UDIVEDeviceAction::GetResolvedDisplayName() const
{
	if (!DisplayName.IsEmpty())
	{
		return DisplayName;
	}

	const UClass* ActionClass = GetClass();
	return ActionClass ? ActionClass->GetDisplayNameText() : FText::GetEmpty();
}

bool UDIVEContinuousDeviceAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	(void)Context;
	return false;
}

void UDIVEContinuousDeviceAction::UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime)
{
	(void)ScreenDelta;
	(void)DeltaTime;
}

void UDIVEContinuousDeviceAction::EndInteraction_Implementation(bool bCommit)
{
	(void)bCommit;
	NotifyInteractionCompleted();
}

void UDIVEContinuousDeviceAction::MarkInteractionActive()
{
	// Catalog-hosted actions are shared instances: at most one interaction can be active at a time.
	// If this fires, two concurrent interactions attempted to use the same action object.
	ensure(!bInteractionActive);
	bInteractionActive = true;
}

void UDIVEContinuousDeviceAction::NotifyInteractionCompleted()
{
	bInteractionActive = false;
}

bool UDIVEContinuousDeviceAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	(void)Context;
	return false;
}
