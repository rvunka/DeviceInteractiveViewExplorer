// Copyright (c) 2026. All Rights Reserved.

#include "DIVEPawnPhysicalDriveProvider.h"

namespace
{
UClass* GRegisteredPhysicalDriveProviderClass = nullptr;
}

void UDIVEPawnPhysicalDriveProvider::InitializeOnPawn(AActor* /*Owner*/)
{
}

void UDIVEPawnPhysicalDriveProvider::ShutdownOnPawn()
{
}

void UDIVEPawnPhysicalDriveProvider::TickDrive(float /*DeltaTime*/)
{
}

namespace DIVEPhysicalDriveExtension
{
void RegisterProviderClass(UClass* ProviderClass)
{
	if (ProviderClass && ProviderClass->HasAnyClassFlags(CLASS_Abstract))
	{
		return;
	}

	GRegisteredPhysicalDriveProviderClass = ProviderClass;
}

UClass* GetRegisteredProviderClass()
{
	return GRegisteredPhysicalDriveProviderClass;
}
}
