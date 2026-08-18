// Copyright (c) 2026. All Rights Reserved.

#include "DIVEPawnPhysicalDriveResolve.h"

#include "DIVEPawnPhysicalDrive.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIVEPawnPhysicalDrive, Log, All);

namespace DIVEPawnPhysicalDriveResolve
{
namespace
{
void CollectImplementors(APawn* Pawn, TArray<UObject*>& OutImplementors)
{
	OutImplementors.Reset();
	if (!Pawn)
	{
		return;
	}

	TArray<UActorComponent*> Components;
	Pawn->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->Implements<UDIVEPawnPhysicalDrive>())
		{
			OutImplementors.Add(Component);
		}
	}

	if (Pawn->Implements<UDIVEPawnPhysicalDrive>())
	{
		OutImplementors.Add(Pawn);
	}
}

IDIVEPawnPhysicalDrive* AsDrive(UObject* Object)
{
	return Object ? Cast<IDIVEPawnPhysicalDrive>(Object) : nullptr;
}
}

IDIVEPawnPhysicalDrive* FindOnPawn(APawn* Pawn, FName ComponentName)
{
	if (!Pawn)
	{
		return nullptr;
	}

	TArray<UObject*> Implementors;
	CollectImplementors(Pawn, Implementors);

	if (!ComponentName.IsNone())
	{
		for (UObject* Object : Implementors)
		{
			if (Object && Object->GetFName() == ComponentName)
			{
				return AsDrive(Object);
			}
		}

		UE_LOG(
			LogDIVEPawnPhysicalDrive,
			Warning,
			TEXT("DIVE pawn physical drive on '%s': no IDIVEPawnPhysicalDrive named '%s'."),
			*GetNameSafe(Pawn),
			*ComponentName.ToString());
		return nullptr;
	}

	if (Implementors.Num() == 1)
	{
		return AsDrive(Implementors[0]);
	}

	if (Implementors.Num() > 1)
	{
		UE_LOG(
			LogDIVEPawnPhysicalDrive,
			Warning,
			TEXT("DIVE pawn physical drive on '%s': %d IDIVEPawnPhysicalDrive implementor(s); named lookup required (no silent first-by-class)."),
			*GetNameSafe(Pawn),
			Implementors.Num());
	}

	return nullptr;
}

IDIVEPawnPhysicalDrive* FindOnPlayerController(APlayerController* PlayerController, FName ComponentName)
{
	if (!PlayerController)
	{
		return nullptr;
	}

	return FindOnPawn(PlayerController->GetPawn(), ComponentName);
}
}
