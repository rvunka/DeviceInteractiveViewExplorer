// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEComponentResolve.h"

#include "GameFramework/Actor.h"

UActorComponent* DIVEComponentResolve::FindComponentByName(AActor* Owner, FName ComponentName)
{
	if (!Owner || ComponentName.IsNone())
	{
		return nullptr;
	}

	TArray<UActorComponent*> Components;
	Owner->GetComponents(Components);

	for (UActorComponent* Component : Components)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}

	return nullptr;
}
