// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEGripLegacyDevQuery.h"

#include "DIVESessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#if DIVE_WITH_GRIP
#include "Hand/GRIPHandComponent.h"
#include "Input/GRIPInputComponent.h"
#endif

#if DIVE_WITH_GRIP_BRIDGE
#include "DIVEGRIPBridgeComponent.h"
#endif

namespace DIVEGripLegacyDevQueryPrivate
{
#if DIVE_WITH_GRIP
	bool IsGripOwningMouseWheel(const AActor* Owner)
	{
		if (!Owner)
		{
			return false;
		}

		TInlineComponentArray<UActorComponent*> Components(Owner);
		for (UActorComponent* Component : Components)
		{
			if (const UGRIPHandComponent* Hand = Cast<UGRIPHandComponent>(Component))
			{
				if (Hand->IsGrabbing() && !Hand->IsManualRotateActive())
				{
					return true;
				}
			}
		}

		return false;
	}
#endif

	const UDIVESessionSubsystem* GetDiveSubsystem(const AActor* Owner)
	{
		if (!Owner)
		{
			return nullptr;
		}

		const UWorld* World = Owner->GetWorld();
		if (!World)
		{
			return nullptr;
		}

		const UGameInstance* GameInstance = World->GetGameInstance();
		return GameInstance ? GameInstance->GetSubsystem<UDIVESessionSubsystem>() : nullptr;
	}

	bool IsPawnBridgePhysicalDriveActive(const AActor* Owner)
	{
		const UDIVESessionSubsystem* DiveSubsystem = GetDiveSubsystem(Owner);
		return DiveSubsystem && DiveSubsystem->IsPawnPhysicalDriveActive();
	}
}

bool DIVEGripLegacyDevQuery::ShouldDeferMouseWheelToGrip(const AActor* Owner)
{
#if DIVE_WITH_GRIP
	return DIVEGripLegacyDevQueryPrivate::IsGripOwningMouseWheel(Owner);
#else
	(void)Owner;
	return false;
#endif
}

bool DIVEGripLegacyDevQuery::TryForwardMouseWheelToGrip(const AActor* Owner, const float WheelDelta)
{
	if (!ShouldDeferMouseWheelToGrip(Owner) || FMath::IsNearlyZero(WheelDelta))
	{
		return false;
	}

#if DIVE_WITH_GRIP_BRIDGE
	if (DIVEGripLegacyDevQueryPrivate::IsPawnBridgePhysicalDriveActive(Owner))
	{
		TInlineComponentArray<UActorComponent*> Components(Owner);
		for (UActorComponent* Component : Components)
		{
			if (UDIVEGRIPBridgeComponent* Bridge = Cast<UDIVEGRIPBridgeComponent>(Component))
			{
				Bridge->ApplyGrabHoldDistanceScroll(WheelDelta);
				return true;
			}
		}
	}
#endif

#if DIVE_WITH_GRIP
	TInlineComponentArray<UActorComponent*> Components(Owner);
	for (UActorComponent* Component : Components)
	{
		if (UGRIPInputComponent* GripInput = Cast<UGRIPInputComponent>(Component))
		{
			GripInput->HandleGrabHoldDistanceScroll(WheelDelta);
			return true;
		}
	}
#else
	(void)Owner;
	(void)WheelDelta;
#endif

	return false;
}
