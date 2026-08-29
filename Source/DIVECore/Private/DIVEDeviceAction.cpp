// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceAction.h"

#include "Engine/World.h"
#include "Internationalization/Text.h"

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

	// Prefer injected ExecutionWorld. Inspectable copies also walk Outer to the actor.
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
	if (!ActionClass)
	{
		return FText::GetEmpty();
	}
#if WITH_EDITOR
	return ActionClass->GetDisplayNameText();
#else
	return FText::FromName(ActionClass->GetFName());
#endif
}

bool UDIVEContinuousDeviceAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	(void)Context;
	return false;
}

void UDIVEContinuousDeviceAction::UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update)
{
	(void)Update;
}

void UDIVEContinuousDeviceAction::EndInteraction_Implementation(bool bCommit)
{
	(void)bCommit;
	NotifyInteractionCompleted();
}

void UDIVEContinuousDeviceAction::MarkInteractionActive(const FDIVEActionContext& Context)
{
	// At most one interaction per action instance (ensure fires if Begin overlaps).
	ensure(!bInteractionActive);
	bInteractionActive = true;
	ActiveInteractionContext = Context;
}

namespace
{
FDIVEInteractionValue ClampNormalized(FDIVEInteractionValue Value)
{
	Value.Normalized = FMath::Clamp(Value.Normalized, 0.f, 1.f);
	return Value;
}

FText FormatNumber(const float Number, const int32 MaxFractionDigits)
{
	FNumberFormattingOptions Options;
	Options.MinimumFractionalDigits = 0;
	Options.MaximumFractionalDigits = MaxFractionDigits;
	Options.RoundingMode = ERoundingMode::HalfFromZero;
	return FText::AsNumber(Number, &Options);
}
}

FText DIVE::FormatInteractionValueReadout(const FText& Label, const FDIVEInteractionValue& Value)
{
	const bool bDomainStyle = !Value.DisplaySuffix.IsEmpty()
		|| (Value.Unit == EDIVEInteractionValueUnit::None && Value.AbsoluteMax > KINDA_SMALL_NUMBER);

	if (bDomainStyle)
	{
		if (Value.AbsoluteMax > KINDA_SMALL_NUMBER)
		{
			if (!Value.DisplaySuffix.IsEmpty())
			{
				return FText::Format(
					NSLOCTEXT("DIVE", "ValueReadoutDomainSpan", "{0}: {1} / {2} {3}"),
					Label,
					FormatNumber(Value.Absolute, 2),
					FormatNumber(Value.AbsoluteMax, 2),
					Value.DisplaySuffix);
			}
			return FText::Format(
				NSLOCTEXT("DIVE", "ValueReadoutDomainSpanNoSuffix", "{0}: {1} / {2}"),
				Label,
				FormatNumber(Value.Absolute, 2),
				FormatNumber(Value.AbsoluteMax, 2));
		}
		return FText::Format(
			NSLOCTEXT("DIVE", "ValueReadoutDomain", "{0}: {1} {2}"),
			Label,
			FormatNumber(Value.Absolute, 2),
			Value.DisplaySuffix);
	}

	switch (Value.Unit)
	{
	case EDIVEInteractionValueUnit::Degrees:
		return FText::Format(
			NSLOCTEXT("DIVE", "ValueReadoutDegrees", "{0}: {1}°"),
			Label,
			FormatNumber(Value.Absolute, 1));
	case EDIVEInteractionValueUnit::Turns:
		if (Value.AbsoluteMax > KINDA_SMALL_NUMBER)
		{
			return FText::Format(
				NSLOCTEXT("DIVE", "ValueReadoutTurnsSpan", "{0}: {1} / {2}"),
				Label,
				FormatNumber(Value.Absolute, 2),
				FormatNumber(Value.AbsoluteMax, 2));
		}
		return FText::Format(
			NSLOCTEXT("DIVE", "ValueReadoutTurns", "{0}: {1}"),
			Label,
			FormatNumber(Value.Absolute, 2));
	case EDIVEInteractionValueUnit::Centimeters:
		if (Value.AbsoluteMax > KINDA_SMALL_NUMBER)
		{
			return FText::Format(
				NSLOCTEXT("DIVE", "ValueReadoutCentimetersSpan", "{0}: {1} / {2} cm"),
				Label,
				FormatNumber(Value.Absolute, 2),
				FormatNumber(Value.AbsoluteMax, 2));
		}
		return FText::Format(
			NSLOCTEXT("DIVE", "ValueReadoutCentimeters", "{0}: {1} cm"),
			Label,
			FormatNumber(Value.Absolute, 2));
	case EDIVEInteractionValueUnit::None:
	default:
		return FText::Format(
			NSLOCTEXT("DIVE", "ValueReadoutNormalized", "{0}: {1}"),
			Label,
			FormatNumber(Value.Normalized, 2));
	}
}

void UDIVEContinuousDeviceAction::NotifyValueChanged(const float NormalizedValue)
{
	FDIVEInteractionValue Value;
	Value.Normalized = NormalizedValue;
	NotifyInteractionValue(Value);
}

void UDIVEContinuousDeviceAction::NotifyInteractionValue(const FDIVEInteractionValue& Value)
{
	OnValueChanged.Broadcast(this, ActiveInteractionContext, ClampNormalized(Value));
}

void UDIVEContinuousDeviceAction::NotifyInteractionCompleted()
{
	bInteractionActive = false;
	ActiveInteractionContext = FDIVEActionContext();
}

bool UDIVEContinuousDeviceAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	(void)Context;
	return false;
}
