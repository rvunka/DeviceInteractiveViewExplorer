// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVEDeviceControlRegistry.h"
#include "DIVEProxyDrive.h"

#include "DIVEProxyDriveTestTypes.generated.h"

UCLASS()
class UDIVETestProxyDriveComponent : public UActorComponent, public IDIVEProxyDrive
{
	GENERATED_BODY()

public:
	virtual bool CanProxyDrive_Implementation(const FDIVEProxyDriveContext& Context) const override
	{
		(void)Context;
		return true;
	}

	virtual bool BeginProxyDrive_Implementation(const FDIVEProxyDriveContext& Context) override
	{
		(void)Context;
		return true;
	}

	virtual void ApplyProxyDriveDelta_Implementation(FVector2D ScreenDelta) override
	{
		(void)ScreenDelta;
	}

	virtual void EndProxyDrive_Implementation(bool bCommit) override
	{
		(void)bCommit;
	}

	virtual bool GetProxyDriveNormalizedValue_Implementation(float& OutNormalizedValue) const override
	{
		OutNormalizedValue = 0.f;
		return false;
	}
};

UCLASS()
class UDIVETestControlRegistryComponent : public UActorComponent, public IDIVEDeviceControlRegistry
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UObject> DriveObject;

	virtual UObject* ResolveProxyDriveObject_Implementation(UPrimitiveComponent* HitComponent) override
	{
		(void)HitComponent;
		return DriveObject;
	}
};
