// Copyright (c) 2026. All Rights Reserved.

#include "DIVEProxyDriveResolve.h"

#include "DIVEDeviceControlRegistry.h"
#include "DIVEHierarchy.h"
#include "DIVEProxyDrive.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogDIVEProxyDriveResolve, Log, All);

namespace DIVEProxyDriveResolve
{
namespace
{
AActor* FindAttachRoot(AActor* Start)
{
	AActor* Root = Start;
	TSet<AActor*> Seen;
	while (Root)
	{
		Seen.Add(Root);
		AActor* Parent = Root->GetAttachParentActor();
		if (!Parent || Seen.Contains(Parent))
		{
			break;
		}
		Root = Parent;
	}
	return Root;
}

void CollectRegistryDrivesOnObject(
	UObject* Object,
	UPrimitiveComponent* HitComponent,
	TArray<UObject*>& OutDriveObjects)
{
	if (!Object || !HitComponent || !Object->GetClass()->ImplementsInterface(UDIVEDeviceControlRegistry::StaticClass()))
	{
		return;
	}

	if (UObject* DriveObject = IDIVEDeviceControlRegistry::Execute_ResolveProxyDriveObject(Object, HitComponent))
	{
		if (DriveObject->GetClass()->ImplementsInterface(UDIVEProxyDrive::StaticClass()))
		{
			OutDriveObjects.AddUnique(DriveObject);
		}
	}
}

IDIVEProxyDrive* AsProxyDrive(UObject* Object)
{
	return Object ? Cast<IDIVEProxyDrive>(Object) : nullptr;
}
}

IDIVEProxyDrive* FindProxyDriveForHit(UPrimitiveComponent* HitComponent)
{
	if (!HitComponent)
	{
		return nullptr;
	}

	AActor* HitOwner = HitComponent->GetOwner();
	if (!HitOwner)
	{
		return nullptr;
	}

	AActor* DeviceRoot = FindAttachRoot(HitOwner);
	TArray<UObject*> RegistryDrives;
	DIVE::ForEachDeviceActor(DeviceRoot, [HitComponent, &RegistryDrives](AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		CollectRegistryDrivesOnObject(Actor, HitComponent, RegistryDrives);

		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			CollectRegistryDrivesOnObject(Component, HitComponent, RegistryDrives);
		}
	});

	if (RegistryDrives.Num() > 1)
	{
		UE_LOG(
			LogDIVEProxyDriveResolve,
			Warning,
			TEXT("DIVE proxy drive: %d IDIVEDeviceControlRegistry results for '%s'; named/single registry required (no silent first)."),
			RegistryDrives.Num(),
			*GetNameSafe(HitComponent));
		return nullptr;
	}
	if (RegistryDrives.Num() == 1)
	{
		return AsProxyDrive(RegistryDrives[0]);
	}

	if (HitOwner->Implements<UDIVEProxyDrive>())
	{
		return Cast<IDIVEProxyDrive>(HitOwner);
	}

	TArray<UActorComponent*> OwnerComponents;
	HitOwner->GetComponents(OwnerComponents);
	TArray<UObject*> DirectDrives;
	for (UActorComponent* Component : OwnerComponents)
	{
		if (Component && Component->Implements<UDIVEProxyDrive>())
		{
			DirectDrives.AddUnique(Component);
		}
	}

	if (DirectDrives.Num() > 1)
	{
		UE_LOG(
			LogDIVEProxyDriveResolve,
			Warning,
			TEXT("DIVE proxy drive on '%s': %d IDIVEProxyDrive component(s); register via IDIVEDeviceControlRegistry (no silent first)."),
			*GetNameSafe(HitOwner),
			DirectDrives.Num());
		return nullptr;
	}
	if (DirectDrives.Num() == 1)
	{
		return AsProxyDrive(DirectDrives[0]);
	}

	return nullptr;
}
}
