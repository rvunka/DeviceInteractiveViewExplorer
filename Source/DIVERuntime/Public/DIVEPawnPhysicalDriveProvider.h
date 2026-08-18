// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "DIVEPawnPhysicalDriveProvider.generated.h"

class AActor;

/**
 * Instanced Physical-drive extension owned by UDIVEPlayerComponent.
 * Sibling plugins register a concrete class at module startup. DIVERuntime does not link GRIP.
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced, CollapseCategories)
class DIVERUNTIME_API UDIVEPawnPhysicalDriveProvider : public UObject
{
	GENERATED_BODY()

public:
	virtual void InitializeOnPawn(AActor* Owner);
	virtual void ShutdownOnPawn();
	virtual void TickDrive(float DeltaTime);
};

namespace DIVEPhysicalDriveExtension
{
	DIVERUNTIME_API void RegisterProviderClass(UClass* ProviderClass);
	DIVERUNTIME_API UClass* GetRegisteredProviderClass();
}
