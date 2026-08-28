// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceAction.h"

#include "DIVEBuiltInActions.generated.h"

class UPrimitiveComponent;

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

/** Session adapter for IDIVEProxyDrive. Hidden from Bindings / Catalog. */
UCLASS(HideDropdown, NotBlueprintable, meta = (DisplayName = "DIVE Proxy Drive Forward Action"))
class DIVERUNTIME_API UDIVEProxyDriveForwardAction : public UDIVEContinuousDeviceAction
{
	GENERATED_BODY()

public:
	UDIVEProxyDriveForwardAction();

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context) override;
	virtual void UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update) override;
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

/** Polar gesture around the picked primitive's rim; edge-on falls back to DegreesPerPixel. */
UCLASS(BlueprintType, EditInlineNew, meta = (
	DisplayName = "DIVE Rotary Drive Action",
	ToolTip = "Drag around the rim to rotate the picked primitive around Axis. Begin isolates a child on a simulating device (unweld, Query Only) without disabling parent physics. Engine cylinder is Z-up: Axis Z is spin-in-place."))
class DIVERUNTIME_API UDIVERotaryDriveAction : public UDIVEContinuousDeviceAction
{
	GENERATED_BODY()

public:
	UDIVERotaryDriveAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ToolTip = "Local axis of the picked primitive. Engine Shape_Cylinder is Z-up; Axis Z is spin-in-place."))
	EDIVEDriveAxis Axis = EDIVEDriveAxis::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.01",
		ToolTip = "Fallback only when the pointer is edge-on to the axis or inside the polar dead zone."))
	float DegreesPerPixel = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	bool bLimitAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		EditCondition = "bLimitAngle",
		ToolTip = "Minimum angle from the part's rest pose (authored relative rotation on first grab), not from each mouse-down. Extra drag past the stop is discarded."))
	float MinAngleDegrees = -180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		EditCondition = "bLimitAngle",
		ToolTip = "Maximum angle from the part's rest pose (authored relative rotation on first grab), not from each mouse-down. Extra drag past the stop is discarded."))
	float MaxAngleDegrees = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.0",
		ToolTip = "0 = no detents. Otherwise snap the applied angle to this step (raw travel still accumulates between detents)."))
	float DetentStepDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	bool bRestoreOnCancel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		ToolTip = "When enabled and the angle is limited, Absolute is lerp(DomainMin, DomainMax, Normalized). Gesture still rotates in degrees."))
	bool bUseDomainReadout = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		EditCondition = "bUseDomainReadout && bLimitAngle",
		EditConditionHides))
	float DomainMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		EditCondition = "bUseDomainReadout && bLimitAngle",
		EditConditionHides))
	float DomainMax = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		EditCondition = "bUseDomainReadout && bLimitAngle",
		EditConditionHides,
		ToolTip = "HUD / Value Event suffix (A, V, Ω, …). Empty keeps domain Absolute without a unit glyph."))
	FText ReadoutSuffix;

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context) override;
	virtual void UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update) override;
	virtual void EndInteraction_Implementation(bool bCommit) override;

	FDIVEInteractionValue MakeInteractionValue() const;

private:
	void ApplyAccumulated();
	float GetAppliedDegrees() const;
	float GetNormalizedValue() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> ActiveTarget;

	/** Rest pose for this primitive; limits and apply are relative to this, not to mouse-down. */
	UPROPERTY(Transient)
	FQuat RestRelativeRotation = FQuat::Identity;

	/** Relative rotation at mouse-down; restored on cancel. */
	UPROPERTY(Transient)
	FRotator StartRelativeRotation = FRotator::ZeroRotator;

	/** From rest; clamped to Min/Max when limited. */
	UPROPERTY(Transient)
	float AccumulatedDegrees = 0.f;

	uint8 SavedCollisionEnabled = 0;
	bool bSavedAutoWeld = false;
	bool bHasSavedIsolation = false;
	/** Frozen at Begin; first Update seeds the live pointer on the plane perpendicular to Axis. */
	FVector FrozenAxisWorld = FVector::ZeroVector;
	FVector AxisOrigin = FVector::ZeroVector;
	FVector PlaneBasisU = FVector::ZeroVector;
	FVector PlaneBasisV = FVector::ZeroVector;
	FVector PreviousPlaneVector = FVector::ZeroVector;
	bool bAwaitingPolarSeed = false;
};

/**
 * Polar unscrew gesture around the picked primitive's rim. Progress is encoded in relative
 * rotation + translation. At TurnsToRelease the part detaches and simulates physics.
 */
UCLASS(BlueprintType, EditInlineNew, meta = (
	DisplayName = "DIVE Threaded Drive Action",
	ToolTip = "Drag around the rim to unscrew the picked primitive around Axis. Begin isolates a child on a simulating device the same way Rotary does. On complete: detach + Simulate Physics."))
class DIVERUNTIME_API UDIVEThreadedDriveAction : public UDIVEContinuousDeviceAction
{
	GENERATED_BODY()

public:
	UDIVEThreadedDriveAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ToolTip = "Local axis of the picked primitive. Engine Shape_Cylinder is Z-up; Axis Z is spin-in-place."))
	EDIVEDriveAxis Axis = EDIVEDriveAxis::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.01",
		ToolTip = "Fallback only when the pointer is edge-on to the axis or inside the polar dead zone."))
	float DegreesPerPixel = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (ClampMin = "0.01"))
	float TurnsToRelease = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.0",
		ToolTip = "Relative translation along Axis per full turn (cm)."))
	float PitchCmPerTurn = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ToolTip = "If true, positive polar delta around Axis (right-hand rule) loosens toward release."))
	bool bPositiveDeltaLoosens = true;

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context) override;
	virtual void UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update) override;
	virtual void EndInteraction_Implementation(bool bCommit) override;

	FDIVEInteractionValue MakeInteractionValue() const;

private:
	void ApplyAccumulated();
	void ReleaseTarget();
	float GetAppliedTurns() const;
	float GetNormalizedValue() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> ActiveTarget;

	/** Rest pose for this primitive; turns and apply are relative to this, not to mouse-down. */
	UPROPERTY(Transient)
	FTransform RestRelativeTransform = FTransform::Identity;

	/** Relative transform at mouse-down; restored on cancel. */
	UPROPERTY(Transient)
	FTransform StartRelativeTransform = FTransform::Identity;

	/** From rest; clamped to 0..TurnsToRelease. */
	UPROPERTY(Transient)
	float AccumulatedTurns = 0.f;

	uint8 SavedCollisionEnabled = 0;
	bool bSavedAutoWeld = false;
	bool bHasSavedIsolation = false;
	/** Frozen at Begin; first Update seeds the live pointer on the plane perpendicular to Axis. */
	FVector FrozenAxisWorld = FVector::ZeroVector;
	FVector AxisOrigin = FVector::ZeroVector;
	FVector PlaneBasisU = FVector::ZeroVector;
	FVector PlaneBasisV = FVector::ZeroVector;
	FVector PreviousPlaneVector = FVector::ZeroVector;
	bool bAwaitingPolarSeed = false;

	UPROPERTY(Transient)
	bool bReleased = false;
};

/**
 * Pointer travel along the picked primitive's local axis. Progress is relative translation.
 * Edge-on / unstable projection falls back to CmPerPixel screen drag.
 */
UCLASS(BlueprintType, EditInlineNew, meta = (
	DisplayName = "DIVE Linear Drive Action",
	ToolTip = "Drag along the rail to slide the picked primitive along Axis. Begin isolates a child on a simulating device the same way Rotary does. Pointer maps 1:1 onto axis centimetres; edge-on falls back to CmPerPixel."))
class DIVERUNTIME_API UDIVELinearDriveAction : public UDIVEContinuousDeviceAction
{
	GENERATED_BODY()

public:
	UDIVELinearDriveAction();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ToolTip = "Local travel axis of the picked primitive (rail direction, not spin)."))
	EDIVEDriveAxis Axis = EDIVEDriveAxis::X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.01",
		ToolTip = "Fallback only when the pointer ray is nearly parallel to the axis or the projection is unstable."))
	float CmPerPixel = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	bool bLimitTravel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		EditCondition = "bLimitTravel",
		ToolTip = "Minimum travel from the part's rest pose (authored relative transform on first grab), not from each mouse-down. Extra drag past the stop is discarded."))
	float MinTravelCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		EditCondition = "bLimitTravel",
		ToolTip = "Maximum travel from the part's rest pose (authored relative transform on first grab), not from each mouse-down. Extra drag past the stop is discarded."))
	float MaxTravelCm = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive", meta = (
		ClampMin = "0.0",
		ToolTip = "0 = no detents. Otherwise snap the applied travel to this step (raw travel still accumulates between detents)."))
	float DetentStepCm = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	bool bRestoreOnCancel = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		ToolTip = "When enabled and travel is limited, Absolute is lerp(DomainMin, DomainMax, Normalized). Gesture still slides in centimetres."))
	bool bUseDomainReadout = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		EditCondition = "bUseDomainReadout && bLimitTravel",
		EditConditionHides))
	float DomainMin = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		EditCondition = "bUseDomainReadout && bLimitTravel",
		EditConditionHides))
	float DomainMax = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive|Readout", meta = (
		EditCondition = "bUseDomainReadout && bLimitTravel",
		EditConditionHides,
		ToolTip = "HUD / Value Event suffix (A, V, Ω, …). Empty keeps domain Absolute without a unit glyph."))
	FText ReadoutSuffix;

	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const override;
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context) override;
	virtual void UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update) override;
	virtual void EndInteraction_Implementation(bool bCommit) override;

	FDIVEInteractionValue MakeInteractionValue() const;

private:
	void ApplyAccumulated();
	float GetAppliedTravelCm() const;
	float GetNormalizedValue() const;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> ActiveTarget;

	/** Rest pose for this primitive; limits and apply are relative to this, not to mouse-down. */
	UPROPERTY(Transient)
	FTransform RestRelativeTransform = FTransform::Identity;

	/** Relative transform at mouse-down; restored on cancel. */
	UPROPERTY(Transient)
	FTransform StartRelativeTransform = FTransform::Identity;

	/** From rest; clamped to Min/Max when limited. */
	UPROPERTY(Transient)
	float AccumulatedTravelCm = 0.f;

	uint8 SavedCollisionEnabled = 0;
	bool bSavedAutoWeld = false;
	bool bHasSavedIsolation = false;
	/** Frozen at Begin; first Update seeds the live pointer parameter on the axis. */
	FVector FrozenAxisWorld = FVector::ZeroVector;
	FVector AxisOrigin = FVector::ZeroVector;
	float PreviousParameterCm = 0.f;
	bool bHasPreviousParameter = false;
	bool bAwaitingLinearSeed = false;
};
