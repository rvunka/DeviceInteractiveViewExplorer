// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceAction.h"

class UPrimitiveComponent;

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

/** Catalog slot with no built-in logic. Succeeds and fires the device DIVE Action Event. */
UCLASS(BlueprintType, EditInlineNew, meta = (
	DisplayName = "DIVE Notify Action",
	ToolTip = "No built-in logic. Succeeds and fires the device DIVE Action Event."))
class DIVERUNTIME_API UDIVENotifyAction : public UDIVEDeviceAction
{
	GENERATED_BODY()

public:
	UDIVENotifyAction();

	virtual bool Execute_Implementation(const FDIVEActionContext& Context) override;
};

UENUM(BlueprintType)
enum class EDIVEDriveAxis : uint8
{
	X,
	Y,
	Z
};

/** Stateless mapper: screen drag → rotation around a local axis. State lives on the target transform. */
UCLASS(BlueprintType, EditInlineNew, meta = (
	DisplayName = "DIVE Rotary Drive Action",
	ToolTip = "Hold/drag rotates the picked primitive around Axis. Limits and detents are per-instance."))
class DIVERUNTIME_API UDIVERotaryDriveAction : public UDIVEContinuousDeviceAction
{
	GENERATED_BODY()

public:
	UDIVERotaryDriveAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	EDIVEDriveAxis Axis = EDIVEDriveAxis::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.01"))
	float DegreesPerPixel = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	bool bLimitAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (EditCondition = "bLimitAngle"))
	float MinAngleDegrees = -180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (EditCondition = "bLimitAngle"))
	float MaxAngleDegrees = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.0",
		ToolTip = "0 = no detents. Otherwise snap Accumulated angle to this step."))
	float DetentStepDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	bool bRestoreOnCancel = true;

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context) override;
	virtual void UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime) override;
	virtual void EndInteraction_Implementation(bool bCommit) override;

	FDIVEInteractionValue MakeInteractionValue() const;

private:
	void ApplyAccumulated();
	float GetNormalizedValue() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> ActiveTarget;

	UPROPERTY(Transient)
	FRotator StartRelativeRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	FRotator ViewRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	float AccumulatedDegrees = 0.f;
};

/**
 * Screen drag unscrews a primitive along its local axis. Progress is encoded in relative
 * rotation + translation. At TurnsToRelease the part detaches and simulates physics.
 */
UCLASS(BlueprintType, EditInlineNew, meta = (
	DisplayName = "DIVE Threaded Drive Action",
	ToolTip = "Hold/drag unscrews the picked primitive. On complete: detach + Simulate Physics."))
class DIVERUNTIME_API UDIVEThreadedDriveAction : public UDIVEContinuousDeviceAction
{
	GENERATED_BODY()

public:
	UDIVEThreadedDriveAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	EDIVEDriveAxis Axis = EDIVEDriveAxis::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.01"))
	float DegreesPerPixel = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.01"))
	float TurnsToRelease = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.0",
		ToolTip = "Relative translation along Axis per full turn (cm)."))
	float PitchCmPerTurn = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ToolTip = "If true, positive mapped delta (typically drag right) loosens toward release."))
	bool bPositiveDeltaLoosens = true;

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context) override;
	virtual void UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime) override;
	virtual void EndInteraction_Implementation(bool bCommit) override;

	FDIVEInteractionValue MakeInteractionValue() const;

private:
	void ApplyAccumulated();
	void ReleaseTarget();
	float GetNormalizedValue() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> ActiveTarget;

	UPROPERTY(Transient)
	FTransform StartRelativeTransform = FTransform::Identity;

	UPROPERTY(Transient)
	FRotator ViewRotation = FRotator::ZeroRotator;

	UPROPERTY(Transient)
	float AccumulatedTurns = 0.f;

	UPROPERTY(Transient)
	bool bReleased = false;
};
