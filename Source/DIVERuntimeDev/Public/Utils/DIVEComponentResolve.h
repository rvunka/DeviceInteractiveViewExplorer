// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "UObject/NameTypes.h"

class AActor;
class UActorComponent;

namespace DIVEComponentResolve
{
	DIVERUNTIMEDEV_API UActorComponent* FindComponentByName(AActor* Owner, FName ComponentName);

	template<typename ComponentType>
	ComponentType* FindComponentByName(AActor* Owner, FName ComponentName)
	{
		if (UActorComponent* Component = FindComponentByName(Owner, ComponentName))
		{
			return Cast<ComponentType>(Component);
		}

		return nullptr;
	}

	template<typename ComponentType>
	ComponentType* FindComponentByNameOrClass(AActor* Owner, FName ComponentName)
	{
		if (ComponentType* NamedComponent = FindComponentByName<ComponentType>(Owner, ComponentName))
		{
			return NamedComponent;
		}

		return Owner ? Owner->FindComponentByClass<ComponentType>() : nullptr;
	}
}
