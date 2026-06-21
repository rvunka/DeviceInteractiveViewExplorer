// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"

namespace DIVE
{
inline bool IsComponentAttachedUnder(const USceneComponent* Component, const USceneComponent* Ancestor)
{
	for (const USceneComponent* Current = Component; Current; Current = Current->GetAttachParent())
	{
		if (Current == Ancestor)
		{
			return true;
		}
	}

	return false;
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

template<typename TComponent>
inline TComponent* FindAncestorComponent(USceneComponent* Component)
{
	for (USceneComponent* Current = Component; Current; Current = Current->GetAttachParent())
	{
		if (TComponent* Match = Cast<TComponent>(Current))
		{
			return Match;
		}
	}

	return nullptr;
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
