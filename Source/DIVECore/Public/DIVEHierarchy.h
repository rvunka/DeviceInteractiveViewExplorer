// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Set.h"
#include "Templates/Function.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

namespace DIVE
{
inline void ForEachDeviceActor(AActor* DeviceHost, TFunctionRef<void(AActor*)> Visitor)
{
	if (!DeviceHost)
	{
		return;
	}

	TSet<AActor*> ProcessedActors;
	TArray<AActor*> ActorStack;
	ActorStack.Add(DeviceHost);

	while (ActorStack.Num() > 0)
	{
		AActor* Actor = ActorStack.Pop(EAllowShrinking::No);
		if (!Actor || ProcessedActors.Contains(Actor))
		{
			continue;
		}

		ProcessedActors.Add(Actor);
		Visitor(Actor);

		TArray<AActor*> AttachedActors;
		Actor->GetAttachedActors(AttachedActors);
		ActorStack.Append(AttachedActors);
	}
}

inline void CollectDevicePrimitives(AActor* DeviceHost, TArray<UPrimitiveComponent*>& OutPrimitives)
{
	OutPrimitives.Reset();
	ForEachDeviceActor(DeviceHost, [&OutPrimitives](AActor* Actor)
	{
		// GetComponents resets its output array — collect per-actor then append.
		TArray<UPrimitiveComponent*> ActorPrimitives;
		Actor->GetComponents<UPrimitiveComponent>(ActorPrimitives);
		OutPrimitives.Append(ActorPrimitives);
	});
}

inline int32 GetAttachDepthToAncestor(const USceneComponent* Component, const USceneComponent* Ancestor)
{
	int32 Depth = 0;
	for (const USceneComponent* Current = Component; Current; Current = Current->GetAttachParent())
	{
		if (Current == Ancestor)
		{
			return Depth;
		}

		++Depth;
	}

	return INDEX_NONE;
}

inline void CollectAttachedPrimitives(USceneComponent* Root, TArray<UPrimitiveComponent*>& OutPrimitives)
{
	if (!Root)
	{
		return;
	}

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Root))
	{
		OutPrimitives.Add(RootPrimitive);
	}

	TArray<USceneComponent*> Stack;
	Stack.Append(Root->GetAttachChildren());
	while (Stack.Num() > 0)
	{
		USceneComponent* Current = Stack.Pop(EAllowShrinking::No);
		if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Current))
		{
			OutPrimitives.Add(Primitive);
		}

		Stack.Append(Current->GetAttachChildren());
	}
}
}
