// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Utils/SharedComponentResolve.h"

class AActor;
class UActorComponent;

namespace DIVEComponentResolve
{
	inline UActorComponent* FindComponentByName(AActor* Owner, FName ComponentName)
	{
		return SharedComponentResolve::FindComponentByName(Owner, ComponentName);
	}

	template<typename ComponentType>
	ComponentType* FindComponentByName(AActor* Owner, FName ComponentName)
	{
		return SharedComponentResolve::FindComponentByName<ComponentType>(Owner, ComponentName);
	}

	template<typename ComponentType>
	ComponentType* FindComponentByNameOrClass(AActor* Owner, FName ComponentName)
	{
		return SharedComponentResolve::FindComponentByNameOrClass<ComponentType>(Owner, ComponentName);
	}

	template<typename InterfaceType>
	UActorComponent* FindInterfaceProvider(
		AActor* Owner,
		FName ProviderName,
		const UActorComponent* ExcludeComponent = nullptr)
	{
		return SharedComponentResolve::FindInterfaceProvider<InterfaceType>(Owner, ProviderName, ExcludeComponent);
	}
}
