// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceAction.h"

#include "DIVEBuiltInActions.generated.h"

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "DIVE Focus Action"))
class DIVERUNTIME_API UDIVEFocusAction : public UDIVEDeviceAction
{
	GENERATED_BODY()

public:
	UDIVEFocusAction();

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool Execute_Implementation(const FDIVEActionContext& Context) override;
};

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "DIVE Isolate Action"))
class DIVERUNTIME_API UDIVEIsolateAction : public UDIVEDeviceAction
{
	GENERATED_BODY()

public:
	UDIVEIsolateAction();

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual FDIVEActionDisplayState GetDisplayState_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool Execute_Implementation(const FDIVEActionContext& Context) override;
};

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "DIVE Simulate Physics Action"))
class DIVERUNTIME_API UDIVESimulatePhysicsAction : public UDIVEDeviceAction
{
	GENERATED_BODY()

public:
	UDIVESimulatePhysicsAction();

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual FDIVEActionDisplayState GetDisplayState_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool Execute_Implementation(const FDIVEActionContext& Context) override;
};

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "DIVE Delete Mesh Action"))
class DIVERUNTIME_API UDIVEDeleteMeshAction : public UDIVEDeviceAction
{
	GENERATED_BODY()

public:
	UDIVEDeleteMeshAction();

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual FDIVEActionDisplayState GetDisplayState_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool Execute_Implementation(const FDIVEActionContext& Context) override;
};

/** Forwards Begin/Delta/End to IDIVEProxyDrive on the pick target (or owner). */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "DIVE Proxy Drive Forward Action"))
class DIVERUNTIME_API UDIVEProxyDriveForwardAction : public UDIVEContinuousDeviceAction
{
	GENERATED_BODY()

public:
	UDIVEProxyDriveForwardAction();

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context) override;
	virtual void UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime) override;
	virtual void EndInteraction_Implementation(bool bCommit) override;

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> ActiveProxyObject;
};
