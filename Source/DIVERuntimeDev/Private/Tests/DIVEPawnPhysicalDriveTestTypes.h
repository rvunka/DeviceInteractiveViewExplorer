// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVEPawnPhysicalDrive.h"
#include "GameFramework/Pawn.h"

#include "DIVEPawnPhysicalDriveTestTypes.generated.h"

UCLASS()
class UDIVETestPawnPhysicalDriveComponent : public UActorComponent, public IDIVEPawnPhysicalDrive
{
	GENERATED_BODY()

public:
	virtual bool CanBeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) const override
	{
		(void)Context;
		return false;
	}

	virtual bool BeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) override
	{
		(void)Context;
		return false;
	}

	virtual void EndPawnPhysicalDrive_Implementation(bool bCommit) override
	{
		(void)bCommit;
	}

	virtual void HandlePawnPhysicalManualRotatePressed_Implementation() override
	{
	}

	virtual void HandlePawnPhysicalManualRotateReleased_Implementation() override
	{
	}

	virtual void HandlePawnPhysicalGrabHoldDistanceScroll_Implementation(float WheelDelta) override
	{
		(void)WheelDelta;
	}
};

UCLASS(NotPlaceable, HideDropdown)
class ADIVETestPhysicalDrivePawn : public APawn, public IDIVEPawnPhysicalDrive
{
	GENERATED_BODY()

public:
	ADIVETestPhysicalDrivePawn()
	{
		AutoPossessPlayer = EAutoReceiveInput::Disabled;
	}

	virtual bool CanBeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) const override
	{
		(void)Context;
		return false;
	}

	virtual bool BeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) override
	{
		(void)Context;
		return false;
	}

	virtual void EndPawnPhysicalDrive_Implementation(bool bCommit) override
	{
		(void)bCommit;
	}

	virtual void HandlePawnPhysicalManualRotatePressed_Implementation() override
	{
	}

	virtual void HandlePawnPhysicalManualRotateReleased_Implementation() override
	{
	}

	virtual void HandlePawnPhysicalGrabHoldDistanceScroll_Implementation(float WheelDelta) override
	{
		(void)WheelDelta;
	}
};
