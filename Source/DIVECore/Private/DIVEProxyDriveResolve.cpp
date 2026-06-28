// Copyright (c) 2026. All Rights Reserved.

#include "DIVEProxyDriveResolve.h"

#include "DIVEProxyDrive.h"
#include "DIVEDeviceControlRegistry.h"
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

	TArray<UActorComponent*> OwnerComponents;
	Owner->GetComponents(OwnerComponents);
	for (UActorComponent* Component : OwnerComponents)
	{
		if (!Component || !Component->Implements<UDIVEDeviceControlRegistry>())
		{
			continue;
		}

		if (UObject* DriveObject = IDIVEDeviceControlRegistry::Execute_ResolveProxyDriveObject(Component, HitComponent))
		{
			if (DriveObject->GetClass()->ImplementsInterface(UDIVEProxyDrive::StaticClass()))
			{
				return Cast<IDIVEProxyDrive>(DriveObject);
			}
		}
	}

	if (Owner->Implements<UDIVEProxyDrive>())
	{
		return Cast<IDIVEProxyDrive>(Owner);
	}

	for (UActorComponent* Component : OwnerComponents)
	{
		if (Component && Component->Implements<UDIVEProxyDrive>())
		{
			return Cast<IDIVEProxyDrive>(Component);
		}
	}

	return nullptr;
}
}
