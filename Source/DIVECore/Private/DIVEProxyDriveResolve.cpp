// Copyright (c) 2026. All Rights Reserved.

#include "DIVEProxyDriveResolve.h"

#include "DIVEProxyDrive.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

namespace DIVEProxyDriveResolve
{
IDIVEProxyDrive* FindProxyDriveForHit(UPrimitiveComponent* HitComponent)
{
	if (!HitComponent)
	{
		return nullptr;
	}

	AActor* Owner = HitComponent->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (Owner->Implements<UDIVEProxyDrive>())
	{
		return Cast<IDIVEProxyDrive>(Owner);
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(Components);
	for (UActorComponent* Component : Components)
	{
		if (Component && Component->Implements<UDIVEProxyDrive>())
		{
			return Cast<IDIVEProxyDrive>(Component);
		}
	}

	return nullptr;
}
}
