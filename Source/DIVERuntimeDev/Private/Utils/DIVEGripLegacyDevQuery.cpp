// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEGripLegacyDevQuery.h"

#include "DIVEPawnPhysicalDrive.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVESessionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

#if DIVE_WITH_GRIP
#include "Hand/GRIPHandComponent.h"
#include "Input/GRIPInputComponent.h"
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

		UWorld* World = Owner->GetWorld();
		return World ? World->GetSubsystem<UDIVESessionSubsystem>() : nullptr;
	}

	bool IsPawnBridgePhysicalDriveActive(const AActor* Owner)
	{
		const UDIVESessionSubsystem* DiveSubsystem = GetDiveSubsystem(Owner);
		return DiveSubsystem && DiveSubsystem->IsPawnPhysicalDriveActive();
	}

	bool ShouldDeferMouseWheelToGrip(const AActor* Owner)
	{
#if DIVE_WITH_GRIP
		return IsGripOwningMouseWheel(Owner);
#else
		(void)Owner;
		return false;
#endif
	}
}

bool DIVEGripLegacyDevQuery::TryForwardMouseWheelToGrip(const AActor* Owner, const float WheelDelta)
{
	if (!DIVEGripLegacyDevQueryPrivate::ShouldDeferMouseWheelToGrip(Owner) || FMath::IsNearlyZero(WheelDelta))
	{
		return false;
	}

	if (DIVEGripLegacyDevQueryPrivate::IsPawnBridgePhysicalDriveActive(Owner))
	{
		APawn* Pawn = const_cast<APawn*>(Cast<APawn>(Owner));
		if (IDIVEPawnPhysicalDrive* Drive = DIVEPawnPhysicalDriveResolve::FindOnPawn(Pawn))
		{
			if (UObject* DriveObject = Cast<UObject>(Drive))
			{
				IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalGrabHoldDistanceScroll(DriveObject, WheelDelta);
				return true;
			}
		}
	}

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
