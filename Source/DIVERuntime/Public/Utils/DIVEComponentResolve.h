// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Utils/SharedComponentResolve.h"

class AActor;

namespace DIVEComponentResolve
{
	template<typename ComponentType>
	ComponentType* FindComponentByNameOrClass(AActor* Owner, FName ComponentName)
	{
		return SharedComponentResolve::FindComponentByNameOrClass<ComponentType>(Owner, ComponentName);
	}
}
