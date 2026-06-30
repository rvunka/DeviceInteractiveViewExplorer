// Copyright (c) 2026. All Rights Reserved.

#include "DIVEPawnPhysicalDriveResolve.h"

#include "DIVEPawnPhysicalDrive.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace DIVEPawnPhysicalDriveResolve
{
IDIVEPawnPhysicalDrive* FindOnPawn(APawn* Pawn)
{
	if (!Pawn)
	{
		return nullptr;
	}

	TArray<UActorComponent*> Components;
	Pawn->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->Implements<UDIVEPawnPhysicalDrive>())
		{
			return Cast<IDIVEPawnPhysicalDrive>(Component);
		}
	}

	if (Pawn->Implements<UDIVEPawnPhysicalDrive>())
	{
		return Cast<IDIVEPawnPhysicalDrive>(Pawn);
	}

	return nullptr;
}

IDIVEPawnPhysicalDrive* FindOnPlayerController(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}

	return FindOnPawn(PlayerController->GetPawn());
}
}
