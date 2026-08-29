// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEGripLegacyDevQuery.h"

#include "DIVEPawnPhysicalDrive.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVESessionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

namespace DIVEGripLegacyDevQueryPrivate
{
	const UDIVESessionSubsystem* GetDiveSubsystem(const AActor* Owner)
	{
		if (!Owner)
		{
			return nullptr;
		}

		UWorld* World = Owner->GetWorld();
		return World ? World->GetSubsystem<UDIVESessionSubsystem>() : nullptr;
	}
}

bool DIVEGripLegacyDevQuery::TryForwardMouseWheelToGrip(const AActor* Owner, const float WheelDelta)
{
	if (!Owner || FMath::IsNearlyZero(WheelDelta))
	{
		return false;
	}

	const UDIVESessionSubsystem* DiveSubsystem = DIVEGripLegacyDevQueryPrivate::GetDiveSubsystem(Owner);
	if (!DiveSubsystem || !DiveSubsystem->IsPawnPhysicalDriveActive())
	{
		return false;
	}

	APawn* Pawn = const_cast<APawn*>(Cast<APawn>(Owner));
	if (IDIVEPawnPhysicalDrive* Drive = DIVEPawnPhysicalDriveResolve::FindOnPawn(Pawn))
	{
		if (UObject* DriveObject = Cast<UObject>(Drive))
		{
			IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalGrabHoldDistanceScroll(DriveObject, WheelDelta);
			return true;
		}
	}

	return false;
}
