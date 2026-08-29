// Copyright (c) 2026. All Rights Reserved.

#include "Actions/DIVEBuiltInActions.h"

#include "Components/PrimitiveComponent.h"
#include "DIVEDriveMapping.h"
#include "DIVESessionSubsystem.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"

namespace
{
UDIVESessionSubsystem* ResolveSessionSubsystem(const UObject* WorldContext, const FDIVEActionContext& Context)
{
	// Device host first: catalog templates have no world; Inspectable copies walk Outer.
	const AActor* Host = Context.DeviceHost.Get();
	UWorld* World = Host ? Host->GetWorld() : nullptr;
	if (!World)
	{
		World = WorldContext ? WorldContext->GetWorld() : nullptr;
	}
	return World ? World->GetSubsystem<UDIVESessionSubsystem>() : nullptr;
}

void ApplyDomainReadout(FDIVEInteractionValue& Value, const float DomainMin, const float DomainMax, const FText& Suffix)
{
	Value.Absolute = FMath::Lerp(DomainMin, DomainMax, Value.Normalized);
	Value.AbsoluteMax = FMath::Abs(DomainMax - DomainMin);
	Value.DisplaySuffix = Suffix;
	Value.Unit = EDIVEInteractionValueUnit::None;
}

float ClampToLimits(const float Value, const bool bLimit, const float BoundA, const float BoundB)
{
	if (!bLimit)
	{
		return Value;
	}
	return FMath::Clamp(Value, FMath::Min(BoundA, BoundB), FMath::Max(BoundA, BoundB));
}

float IntegrateAgainstLimits(const float Accumulated, const float Delta, const bool bLimit, const float BoundA, const float BoundB)
{
	return ClampToLimits(Accumulated + Delta, bLimit, BoundA, BoundB);
}
} // namespace

UDIVEFocusAction::UDIVEFocusAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuFocus", "Focus");
	Presentation = EDIVEActionPresentation::Session;
}

bool UDIVEFocusAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	return Context.PickTarget.Kind == EDIVEFocusKind::Primitive && Context.PickTarget.IsValidFocus();
}

bool UDIVEFocusAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	return Subsystem->FocusTarget(Context.PickTarget, true);
}

UDIVEIsolateAction::UDIVEIsolateAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuIsolate", "Isolate");
	Presentation = EDIVEActionPresentation::Session;
}

bool UDIVEIsolateAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	return Subsystem
		&& Subsystem->IsSessionActive()
		&& Context.PickTarget.Kind == EDIVEFocusKind::Primitive
		&& Context.PickTarget.IsValidFocus();
}

FDIVEActionDisplayState UDIVEIsolateAction::GetDisplayState_Implementation(const FDIVEActionContext& Context) const
{
	FDIVEActionDisplayState State = Super::GetDisplayState_Implementation(Context);
	if (const UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context))
	{
		State.bChecked = Subsystem->IsIsolationActiveForTarget(Context.PickTarget);
	}
	return State;
}

bool UDIVEIsolateAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	return Subsystem->ToggleIsolationForTarget(Context.PickTarget);
}

UDIVESimulatePhysicsAction::UDIVESimulatePhysicsAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuSimulatePhysics", "Simulate Physics");
}

bool UDIVESimulatePhysicsAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	return UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed()
		&& Context.PickTarget.Kind == EDIVEFocusKind::Primitive
		&& Context.Target != nullptr;
}

FDIVEActionDisplayState UDIVESimulatePhysicsAction::GetDisplayState_Implementation(const FDIVEActionContext& Context) const
{
	FDIVEActionDisplayState State = Super::GetDisplayState_Implementation(Context);
	State.bVisible = UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed();
	State.bChecked = Context.Target && Context.Target->IsSimulatingPhysics();
	return State;
}

bool UDIVESimulatePhysicsAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	return Subsystem->ToggleMeshPhysicsForTarget(Context.PickTarget);
}

UDIVEDeleteMeshAction::UDIVEDeleteMeshAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuDeleteMesh", "Delete Mesh");
}

bool UDIVEDeleteMeshAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	return UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed()
		&& Context.PickTarget.Kind == EDIVEFocusKind::Primitive
		&& Context.Target != nullptr;
}

FDIVEActionDisplayState UDIVEDeleteMeshAction::GetDisplayState_Implementation(const FDIVEActionContext& Context) const
{
	FDIVEActionDisplayState State = Super::GetDisplayState_Implementation(Context);
	State.bVisible = UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed();
	return State;
}

bool UDIVEDeleteMeshAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	return Subsystem->DeleteMeshForTarget(Context.PickTarget);
}

UDIVENotifyAction::UDIVENotifyAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuNotify", "Notify");
}

bool UDIVENotifyAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	if (!CanExecute(Context))
	{
		return false;
	}

	return true;
}

UDIVEMomentaryPressAction::UDIVEMomentaryPressAction()
{
	DisplayName = NSLOCTEXT("DIVE", "MomentaryPress", "Press");
}

bool UDIVEMomentaryPressAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	if (!CanExecute(Context))
	{
		return false;
	}

	NotifyValueChanged(1.f);
	return true;
}

void UDIVEMomentaryPressAction::UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update)
{
	(void)Update;
}

void UDIVEMomentaryPressAction::EndInteraction_Implementation(const bool bCommit)
{
	(void)bCommit;
	if (IsInteractionActive())
	{
		NotifyValueChanged(0.f);
	}
	NotifyInteractionCompleted();
}

namespace
{
FVector DriveAxisLocal(const EDIVEDriveAxis Axis)
{
	switch (Axis)
	{
	case EDIVEDriveAxis::X: return FVector::XAxisVector;
	case EDIVEDriveAxis::Y: return FVector::YAxisVector;
	default: return FVector::ZAxisVector;
	}
}

FVector DriveAxisWorld(const UPrimitiveComponent* Target, const EDIVEDriveAxis Axis)
{
	if (!Target)
	{
		return FVector::ZAxisVector;
	}
	return Target->GetComponentTransform().TransformVectorNoScale(DriveAxisLocal(Axis)).GetSafeNormal();
}

void IsolateDrivenPrimitive(UPrimitiveComponent* Target, uint8& OutSavedCollision, bool& OutSavedAutoWeld, bool& OutHasSaved)
{
	if (!Target)
	{
		return;
	}

	OutSavedCollision = static_cast<uint8>(Target->GetCollisionEnabled());
	OutSavedAutoWeld = Target->BodyInstance.bAutoWeld;
	OutHasSaved = true;

	Target->BodyInstance.bAutoWeld = false;

	if (Target->Mobility != EComponentMobility::Movable)
	{
		Target->SetMobility(EComponentMobility::Movable);
	}

	if (Target->IsWelded())
	{
		Target->UnWeldFromParent();
	}

	if (Target->BodyInstance.bSimulatePhysics)
	{
		Target->SetSimulatePhysics(false);
	}

	Target->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	for (USceneComponent* Ancestor = Target->GetAttachParent(); Ancestor; Ancestor = Ancestor->GetAttachParent())
	{
		if (UPrimitiveComponent* Prim = Cast<UPrimitiveComponent>(Ancestor))
		{
			if (Prim->BodyInstance.bSimulatePhysics)
			{
				Prim->SetAllPhysicsLinearVelocity(FVector::ZeroVector);
				Prim->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector);
				Prim->PutRigidBodyToSleep();
				break;
			}
		}
	}
}

void RestoreDrivenPrimitiveIsolation(
	UPrimitiveComponent* Target,
	const uint8 SavedCollision,
	const bool bSavedAutoWeld,
	const bool bHasSaved)
{
	if (!Target || !bHasSaved)
	{
		return;
	}

	Target->BodyInstance.bAutoWeld = bSavedAutoWeld;
	Target->SetCollisionEnabled(static_cast<ECollisionEnabled::Type>(SavedCollision));
}

void SetDrivenRelativeRotation(UPrimitiveComponent* Target, const FQuat& NewRelativeQuat)
{
	if (Target)
	{
		Target->SetRelativeRotation(NewRelativeQuat, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void SetDrivenRelativeTransform(UPrimitiveComponent* Target, const FTransform& NewRelative)
{
	if (Target)
	{
		Target->SetRelativeTransform(NewRelative, false, nullptr, ETeleportType::TeleportPhysics);
	}
}

void ResetDrivenRestToCurrent(FDrivenPrimitiveRest& Rest, const UPrimitiveComponent* Target)
{
	Rest.RelativeTransform = Target->GetRelativeTransform();
	Rest.CommittedAngleDegrees = 0.f;
	Rest.CommittedTurns = 0.f;
}

float RotationDeltaDegrees(const FQuat& From, const FQuat& To)
{
	FVector DeltaAxis = FVector::ZeroVector;
	float DeltaRad = 0.f;
	(From.Inverse() * To).ToAxisAndAngle(DeltaAxis, DeltaRad);
	return FMath::Abs(FMath::UnwindDegrees(FMath::RadiansToDegrees(DeltaRad)));
}

bool IsRotationAroundAxis(const FQuat& From, const FQuat& To, const FVector& AxisLocal)
{
	FVector DeltaAxis = FVector::ZeroVector;
	float DeltaRad = 0.f;
	(From.Inverse() * To).ToAxisAndAngle(DeltaAxis, DeltaRad);
	const float Deg = FMath::Abs(FMath::UnwindDegrees(FMath::RadiansToDegrees(DeltaRad)));
	if (Deg <= 1.f || DeltaAxis.IsNearlyZero())
	{
		return true;
	}
	return FMath::Abs(FVector::DotProduct(DeltaAxis.GetSafeNormal(), AxisLocal.GetSafeNormal())) >= 0.98f;
}

bool IsOffsetAlongAxis(const FVector& From, const FVector& To, const FVector& AxisInParent)
{
	const FVector Axis = AxisInParent.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return false;
	}
	const FVector Delta = To - From;
	return (Delta - Axis * FVector::DotProduct(Delta, Axis)).Size() <= 0.1f;
}

// Recapture only when the part left this action's DOF. Full-transform Equals is unsafe at ±180.
void RecaptureLinearRestIfReoriented(FDrivenPrimitiveRest& Rest, const UPrimitiveComponent* Target)
{
	if (!Target)
	{
		return;
	}
	if (RotationDeltaDegrees(Rest.RelativeTransform.GetRotation(), Target->GetRelativeTransform().GetRotation()) > 1.f)
	{
		ResetDrivenRestToCurrent(Rest, Target);
	}
}

void RecaptureThreadedRestIfOffScrew(
	FDrivenPrimitiveRest& Rest,
	const UPrimitiveComponent* Target,
	const EDIVEDriveAxis Axis)
{
	if (!Target)
	{
		return;
	}

	const FTransform Current = Target->GetRelativeTransform();
	const FVector AxisLocal = DriveAxisLocal(Axis);
	if (!IsRotationAroundAxis(Rest.RelativeTransform.GetRotation(), Current.GetRotation(), AxisLocal))
	{
		ResetDrivenRestToCurrent(Rest, Target);
		return;
	}

	const FVector AxisInParent = Rest.RelativeTransform.TransformVectorNoScale(AxisLocal).GetSafeNormal();
	if (!IsOffsetAlongAxis(Rest.RelativeTransform.GetLocation(), Current.GetLocation(), AxisInParent))
	{
		ResetDrivenRestToCurrent(Rest, Target);
	}
}

float SignedDegreesAroundAxis(const FQuat& From, const FQuat& To, const FVector& AxisUnit)
{
	const FVector Axis = AxisUnit.GetSafeNormal();
	if (Axis.IsNearlyZero())
	{
		return 0.f;
	}

	const FQuat Delta = From.Inverse() * To;
	FVector DeltaAxis = FVector::ZeroVector;
	float DeltaRad = 0.f;
	Delta.ToAxisAndAngle(DeltaAxis, DeltaRad);
	if (DeltaAxis.IsNearlyZero())
	{
		return 0.f;
	}

	float Degrees = FMath::RadiansToDegrees(DeltaRad);
	if (FVector::DotProduct(DeltaAxis.GetSafeNormal(), Axis) < 0.f)
	{
		Degrees = -Degrees;
	}
	return FMath::UnwindDegrees(Degrees);
}

float UnwrapDegreesTowards(const float PrincipalDegrees, const float TowardsDegrees)
{
	return PrincipalDegrees + 360.f * FMath::RoundToFloat((TowardsDegrees - PrincipalDegrees) / 360.f);
}

float SeedUnwrappedDegrees(const float PrincipalDegrees, const float CommittedDegrees)
{
	const float Unwrapped = UnwrapDegreesTowards(PrincipalDegrees, CommittedDegrees);
	const float CommittedPrincipal = FMath::UnwindDegrees(CommittedDegrees);
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(PrincipalDegrees, CommittedPrincipal)) <= 2.f)
	{
		return Unwrapped;
	}
	return PrincipalDegrees;
}

float SeedTravelAlongAxis(const FTransform& Rest, const FTransform& Current, const FVector& AxisLocal)
{
	const FVector AxisInParent = Rest.TransformVectorNoScale(AxisLocal).GetSafeNormal();
	if (AxisInParent.IsNearlyZero())
	{
		return 0.f;
	}
	return FVector::DotProduct(Current.GetLocation() - Rest.GetLocation(), AxisInParent);
}

constexpr float PolarDeadRadiusCm = 1.f;

void ClearPolarGestureState(
	FVector& FrozenAxisWorld,
	FVector& AxisOrigin,
	FVector& PlaneBasisU,
	FVector& PlaneBasisV,
	FVector& PreviousPlaneVector,
	bool& bAwaitingPolarSeed)
{
	FrozenAxisWorld = FVector::ZeroVector;
	AxisOrigin = FVector::ZeroVector;
	PlaneBasisU = FVector::ZeroVector;
	PlaneBasisV = FVector::ZeroVector;
	PreviousPlaneVector = FVector::ZeroVector;
	bAwaitingPolarSeed = false;
}

void InitializePolarGestureState(
	const FDIVEActionContext& Context,
	UPrimitiveComponent* Target,
	const EDIVEDriveAxis Axis,
	FVector& OutFrozenAxisWorld,
	FVector& OutAxisOrigin,
	FVector& OutPlaneBasisU,
	FVector& OutPlaneBasisV,
	FVector& OutPreviousPlaneVector,
	bool& bOutAwaitingPolarSeed)
{
	const FVector SeedPoint = Context.PickHit.bBlockingHit
		? FVector(Context.PickHit.ImpactPoint)
		: Target->GetComponentLocation();
	OutFrozenAxisWorld = DriveAxisWorld(Target, Axis);
	OutAxisOrigin = DIVE::ProjectPointOntoAxis(SeedPoint, Target->GetComponentLocation(), OutFrozenAxisWorld);
	DIVE::BuildAxisPlaneBasis(OutFrozenAxisWorld, OutPlaneBasisU, OutPlaneBasisV);
	// Live pointer seeds on the first Update. Using PickHit as previous would jump
	// when the gesture starts from the context menu (cursor is off the rim).
	OutPreviousPlaneVector = FVector::ZeroVector;
	bOutAwaitingPolarSeed = true;
}

float ResolvePolarDeltaDegrees(
	const FDIVEInteractionUpdate& Update,
	const FVector& AxisOrigin,
	const FVector& FrozenAxisWorld,
	const FVector& PlaneBasisU,
	const FVector& PlaneBasisV,
	FVector& InOutPreviousPlaneVector,
	bool& bInOutAwaitingPolarSeed,
	const float DegreesPerPixel)
{
	const FDIVEPointerAxisAngleResult Polar = DIVE::MapPointerToAxisAngle(
		AxisOrigin,
		FrozenAxisWorld,
		PlaneBasisU,
		PlaneBasisV,
		Update.ViewLocation,
		Update.PickRayDir,
		InOutPreviousPlaneVector,
		PolarDeadRadiusCm);

	const bool bHaveRimHit = Polar.PlaneVector.SizeSquared() >= FMath::Square(PolarDeadRadiusCm);

	if (bInOutAwaitingPolarSeed)
	{
		InOutPreviousPlaneVector = bHaveRimHit ? Polar.PlaneVector : FVector::ZeroVector;
		bInOutAwaitingPolarSeed = false;
		return 0.f;
	}

	if (Polar.bApplied)
	{
		InOutPreviousPlaneVector = Polar.PlaneVector;
		return Polar.DeltaDegrees;
	}

	if (bHaveRimHit && InOutPreviousPlaneVector.IsNearlyZero())
	{
		InOutPreviousPlaneVector = Polar.PlaneVector;
		return 0.f;
	}

	// Lost a stable rim hit (dead zone, grazing, behind-camera). Drop the latch so the
	// next valid hit re-seeds instead of applying a jump from the last good angle.
	InOutPreviousPlaneVector = FVector::ZeroVector;
	return DIVE::MapScreenDeltaToAxisAngle(
		Update.ScreenDelta,
		Update.ViewRotation,
		FrozenAxisWorld,
		DegreesPerPixel);
}

}

void FDrivenRestStore::Compact()
{
	for (auto It = Rests.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

FDrivenPrimitiveRest& FDrivenRestStore::GetOrCapture(const UPrimitiveComponent* Target)
{
	Compact();
	const TWeakObjectPtr<const UPrimitiveComponent> Key(Target);
	if (FDrivenPrimitiveRest* Found = Rests.Find(Key))
	{
		return *Found;
	}

	FDrivenPrimitiveRest Added;
	if (Target)
	{
		Added.RelativeTransform = Target->GetRelativeTransform();
	}
	return Rests.Add(Key, MoveTemp(Added));
}

FDrivenPrimitiveRest* FDrivenRestStore::Find(const UPrimitiveComponent* Target)
{
	if (!Target)
	{
		return nullptr;
	}
	return Rests.Find(TWeakObjectPtr<const UPrimitiveComponent>(Target));
}

void FDrivenRestStore::CommitAngle(const UPrimitiveComponent* Target, const float AngleDegrees)
{
	if (FDrivenPrimitiveRest* Found = Find(Target))
	{
		Found->CommittedAngleDegrees = AngleDegrees;
	}
}

void FDrivenRestStore::CommitTurns(const UPrimitiveComponent* Target, const float Turns)
{
	if (FDrivenPrimitiveRest* Found = Find(Target))
	{
		Found->CommittedTurns = Turns;
	}
}

bool UDIVEPolarDriveAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	return Super::CanExecute_Implementation(Context)
		&& Context.Target != nullptr;
}

bool UDIVEPolarDriveAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	UPrimitiveComponent* Target = Context.Target.Get();
	if (!Target)
	{
		return false;
	}

	RestStore.Compact();
	IsolateDrivenPrimitive(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	ActiveTarget = Target;
	if (!SeedPolarFromRest(Target))
	{
		RestoreDrivenPrimitiveIsolation(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
		bHasSavedIsolation = false;
		ActiveTarget.Reset();
		return false;
	}

	if (!IsInteractionActive())
	{
		ActiveTarget.Reset();
		return true;
	}

	InitializePolarGestureState(
		Context,
		Target,
		Axis,
		FrozenAxisWorld,
		AxisOrigin,
		PlaneBasisU,
		PlaneBasisV,
		PreviousPlaneVector,
		bAwaitingPolarSeed);
	NotifyPolarValue();
	return true;
}

void UDIVEPolarDriveAction::UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update)
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target || !IsInteractionActive() || ShouldStopPolarUpdates())
	{
		return;
	}

	const float DeltaDeg = ResolvePolarDeltaDegrees(
		Update,
		AxisOrigin,
		FrozenAxisWorld,
		PlaneBasisU,
		PlaneBasisV,
		PreviousPlaneVector,
		bAwaitingPolarSeed,
		DegreesPerPixel);
	if (FMath::IsNearlyZero(DeltaDeg))
	{
		return;
	}

	ApplyPolarDeltaDegrees(DeltaDeg);
	if (IsInteractionActive())
	{
		NotifyPolarValue();
	}
}

void UDIVEPolarDriveAction::EndInteraction_Implementation(const bool bCommit)
{
	if (UPrimitiveComponent* Target = ActiveTarget.Get())
	{
		if (bCommit)
		{
			CommitPolarRest(Target);
		}
		else
		{
			RestorePolarOnCancel(Target);
		}
		if (ShouldRestoreIsolationOnEnd())
		{
			RestoreDrivenPrimitiveIsolation(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
		}
	}

	bHasSavedIsolation = false;
	ActiveTarget.Reset();
	ClearPolarAccumulated();
	ClearPolarGestureState(
		FrozenAxisWorld,
		AxisOrigin,
		PlaneBasisU,
		PlaneBasisV,
		PreviousPlaneVector,
		bAwaitingPolarSeed);
	NotifyInteractionCompleted();
}

UDIVERotaryDriveAction::UDIVERotaryDriveAction()
{
	DisplayName = NSLOCTEXT("DIVE", "RotaryDrive", "Rotate");
}

bool UDIVERotaryDriveAction::SeedPolarFromRest(UPrimitiveComponent* Target)
{
	if (!Target)
	{
		return false;
	}

	StartRelativeRotation = Target->GetRelativeRotation();
	FDrivenPrimitiveRest& RestState = RestStore.GetOrCapture(Target);
	RestRelativeRotation = RestState.RelativeTransform.GetRotation();
	AccumulatedDegrees = SeedUnwrappedDegrees(
		SignedDegreesAroundAxis(
			RestRelativeRotation,
			Target->GetRelativeRotation().Quaternion(),
			DriveAxisLocal(Axis)),
		RestState.CommittedAngleDegrees);
	AccumulatedDegrees = ClampToLimits(
		AccumulatedDegrees,
		bLimitAngle,
		MinAngleDegrees,
		MaxAngleDegrees);
	ApplyAccumulated();
	return true;
}

void UDIVERotaryDriveAction::ApplyPolarDeltaDegrees(const float DeltaDegrees)
{
	AccumulatedDegrees = IntegrateAgainstLimits(
		AccumulatedDegrees,
		DeltaDegrees,
		bLimitAngle,
		MinAngleDegrees,
		MaxAngleDegrees);
	ApplyAccumulated();
}

void UDIVERotaryDriveAction::CommitPolarRest(UPrimitiveComponent* Target)
{
	RestStore.CommitAngle(Target, GetAppliedDegrees());
}

void UDIVERotaryDriveAction::RestorePolarOnCancel(UPrimitiveComponent* Target)
{
	if (bRestoreOnCancel)
	{
		SetDrivenRelativeRotation(Target, FQuat(StartRelativeRotation));
	}
}

void UDIVERotaryDriveAction::NotifyPolarValue()
{
	NotifyInteractionValue(MakeInteractionValue());
}

void UDIVERotaryDriveAction::ClearPolarAccumulated()
{
	AccumulatedDegrees = 0.f;
}

void UDIVERotaryDriveAction::ApplyAccumulated()
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target)
	{
		return;
	}

	const FQuat DeltaQ(DriveAxisLocal(Axis), FMath::DegreesToRadians(GetAppliedDegrees()));
	const FQuat NewRelQuat = RestRelativeRotation * DeltaQ;
	SetDrivenRelativeRotation(Target, NewRelQuat);
}

float UDIVERotaryDriveAction::GetAppliedDegrees() const
{
	float Applied = AccumulatedDegrees;
	if (DetentStepDegrees > KINDA_SMALL_NUMBER)
	{
		Applied = FMath::GridSnap(Applied, DetentStepDegrees);
	}
	if (bLimitAngle)
	{
		const float Lo = FMath::Min(MinAngleDegrees, MaxAngleDegrees);
		const float Hi = FMath::Max(MinAngleDegrees, MaxAngleDegrees);
		Applied = FMath::Clamp(Applied, Lo, Hi);
	}
	return Applied;
}

float UDIVERotaryDriveAction::GetNormalizedValue() const
{
	const float Applied = GetAppliedDegrees();
	if (!bLimitAngle)
	{
		return FMath::Fmod(FMath::Abs(Applied), 360.f) / 360.f;
	}

	const float Lo = FMath::Min(MinAngleDegrees, MaxAngleDegrees);
	const float Hi = FMath::Max(MinAngleDegrees, MaxAngleDegrees);
	const float Span = Hi - Lo;
	if (Span <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return (Applied - Lo) / Span;
}

FDIVEInteractionValue UDIVERotaryDriveAction::MakeInteractionValue() const
{
	FDIVEInteractionValue Value;
	Value.Normalized = GetNormalizedValue();
	if (bUseDomainReadout && bLimitAngle)
	{
		ApplyDomainReadout(Value, DomainMin, DomainMax, ReadoutSuffix);
	}
	else
	{
		Value.Absolute = GetAppliedDegrees();
		Value.AbsoluteMax = 0.f;
		Value.Unit = EDIVEInteractionValueUnit::Degrees;
	}
	return Value;
}

UDIVEThreadedDriveAction::UDIVEThreadedDriveAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ThreadedDrive", "Unscrew");
	DegreesPerPixel = 0.35f;
}

bool UDIVEThreadedDriveAction::SeedPolarFromRest(UPrimitiveComponent* Target)
{
	if (!Target)
	{
		return false;
	}

	StartRelativeTransform = Target->GetRelativeTransform();
	FDrivenPrimitiveRest& RestState = RestStore.GetOrCapture(Target);
	RecaptureThreadedRestIfOffScrew(RestState, Target, Axis);
	RestRelativeTransform = RestState.RelativeTransform;
	if (PitchCmPerTurn > KINDA_SMALL_NUMBER)
	{
		AccumulatedTurns = SeedTravelAlongAxis(
			RestRelativeTransform,
			StartRelativeTransform,
			DriveAxisLocal(Axis)) / PitchCmPerTurn;
	}
	else
	{
		const float PrincipalDegrees = SignedDegreesAroundAxis(
			RestRelativeTransform.GetRotation(),
			StartRelativeTransform.GetRotation(),
			DriveAxisLocal(Axis));
		AccumulatedTurns = SeedUnwrappedDegrees(PrincipalDegrees, RestState.CommittedTurns * 360.f) / 360.f;
	}
	AccumulatedTurns = ClampToLimits(AccumulatedTurns, true, 0.f, TurnsToRelease);
	bReleased = false;
	ApplyAccumulated();
	if (GetAppliedTurns() >= TurnsToRelease - KINDA_SMALL_NUMBER)
	{
		ReleaseTarget();
	}
	return true;
}

void UDIVEThreadedDriveAction::ApplyPolarDeltaDegrees(const float DeltaDegrees)
{
	const float SignedTurns = (DeltaDegrees / 360.f) * (bPositiveDeltaLoosens ? 1.f : -1.f);
	AccumulatedTurns = IntegrateAgainstLimits(AccumulatedTurns, SignedTurns, true, 0.f, TurnsToRelease);
	ApplyAccumulated();
	if (GetAppliedTurns() >= TurnsToRelease - KINDA_SMALL_NUMBER)
	{
		ReleaseTarget();
	}
}

void UDIVEThreadedDriveAction::CommitPolarRest(UPrimitiveComponent* Target)
{
	RestStore.CommitTurns(Target, GetAppliedTurns());
}

void UDIVEThreadedDriveAction::RestorePolarOnCancel(UPrimitiveComponent* Target)
{
	if (!bReleased && bRestoreOnCancel)
	{
		SetDrivenRelativeTransform(Target, StartRelativeTransform);
	}
}

void UDIVEThreadedDriveAction::NotifyPolarValue()
{
	NotifyInteractionValue(MakeInteractionValue());
}

void UDIVEThreadedDriveAction::ClearPolarAccumulated()
{
	AccumulatedTurns = 0.f;
	bReleased = false;
}

bool UDIVEThreadedDriveAction::ShouldStopPolarUpdates() const
{
	return bReleased;
}

bool UDIVEThreadedDriveAction::ShouldRestoreIsolationOnEnd() const
{
	return !bReleased;
}

void UDIVEThreadedDriveAction::ApplyAccumulated()
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector AxisLocal = DriveAxisLocal(Axis);
	const float AppliedTurns = GetAppliedTurns();
	const FQuat DeltaQ(AxisLocal, FMath::DegreesToRadians(AppliedTurns * 360.f));
	const FVector AxisInParent = RestRelativeTransform.TransformVectorNoScale(AxisLocal).GetSafeNormal();
	const FVector RelOffset = AxisInParent * (AppliedTurns * PitchCmPerTurn);

	FTransform NewRel = RestRelativeTransform;
	NewRel.SetRotation(RestRelativeTransform.GetRotation() * DeltaQ);
	NewRel.SetTranslation(RestRelativeTransform.GetTranslation() + RelOffset);
	SetDrivenRelativeTransform(Target, NewRel);
}

void UDIVEThreadedDriveAction::ReleaseTarget()
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target || bReleased)
	{
		return;
	}

	bReleased = true;
	RestoreDrivenPrimitiveIsolation(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	bHasSavedIsolation = false;
	Target->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	Target->SetSimulatePhysics(true);
	NotifyInteractionValue(MakeInteractionValue());
	NotifyInteractionCompleted();
}

float UDIVEThreadedDriveAction::GetAppliedTurns() const
{
	return FMath::Clamp(AccumulatedTurns, 0.f, TurnsToRelease);
}

float UDIVEThreadedDriveAction::GetNormalizedValue() const
{
	if (TurnsToRelease <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}
	return FMath::Clamp(GetAppliedTurns() / TurnsToRelease, 0.f, 1.f);
}

FDIVEInteractionValue UDIVEThreadedDriveAction::MakeInteractionValue() const
{
	FDIVEInteractionValue Value;
	Value.Normalized = GetNormalizedValue();
	Value.Absolute = GetAppliedTurns();
	Value.AbsoluteMax = TurnsToRelease;
	Value.Unit = EDIVEInteractionValueUnit::Turns;
	return Value;
}

UDIVELinearDriveAction::UDIVELinearDriveAction()
{
	DisplayName = NSLOCTEXT("DIVE", "LinearDrive", "Slide");
}

bool UDIVELinearDriveAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	return Super::CanExecute_Implementation(Context)
		&& Context.Target != nullptr;
}

namespace
{
void ClearLinearGestureState(
	FVector& FrozenAxisWorld,
	FVector& AxisOrigin,
	float& PreviousParameterCm,
	bool& bHasPreviousParameter,
	bool& bAwaitingLinearSeed)
{
	FrozenAxisWorld = FVector::ZeroVector;
	AxisOrigin = FVector::ZeroVector;
	PreviousParameterCm = 0.f;
	bHasPreviousParameter = false;
	bAwaitingLinearSeed = false;
}

void InitializeLinearGestureState(
	const FDIVEActionContext& Context,
	UPrimitiveComponent* Target,
	const EDIVEDriveAxis Axis,
	FVector& OutFrozenAxisWorld,
	FVector& OutAxisOrigin,
	float& OutPreviousParameterCm,
	bool& bOutHasPreviousParameter,
	bool& bOutAwaitingLinearSeed)
{
	const FVector SeedPoint = Context.PickHit.bBlockingHit
		? FVector(Context.PickHit.ImpactPoint)
		: Target->GetComponentLocation();
	OutFrozenAxisWorld = DriveAxisWorld(Target, Axis);
	OutAxisOrigin = DIVE::ProjectPointOntoAxis(SeedPoint, Target->GetComponentLocation(), OutFrozenAxisWorld);
	// Live pointer seeds on the first Update. Using PickHit as previous would jump
	// when the gesture starts from the context menu (cursor is off the rail).
	OutPreviousParameterCm = 0.f;
	bOutHasPreviousParameter = false;
	bOutAwaitingLinearSeed = true;
}

float ResolveLinearDeltaCm(
	const FDIVEInteractionUpdate& Update,
	const FVector& AxisOrigin,
	const FVector& FrozenAxisWorld,
	float& InOutPreviousParameterCm,
	bool& bInOutHasPreviousParameter,
	bool& bInOutAwaitingLinearSeed,
	const float CmPerPixel)
{
	const FDIVEPointerAxisTravelResult Travel = DIVE::MapPointerToAxisTravel(
		AxisOrigin,
		FrozenAxisWorld,
		Update.ViewLocation,
		Update.PickRayDir,
		bInOutHasPreviousParameter,
		InOutPreviousParameterCm);

	if (bInOutAwaitingLinearSeed)
	{
		if (Travel.bHasParameter)
		{
			InOutPreviousParameterCm = Travel.ParameterCm;
			bInOutHasPreviousParameter = true;
		}
		bInOutAwaitingLinearSeed = false;
		return 0.f;
	}

	if (Travel.bApplied)
	{
		InOutPreviousParameterCm = Travel.ParameterCm;
		bInOutHasPreviousParameter = true;
		return Travel.DeltaCm;
	}

	if (Travel.bHasParameter && !bInOutHasPreviousParameter)
	{
		InOutPreviousParameterCm = Travel.ParameterCm;
		bInOutHasPreviousParameter = true;
		return 0.f;
	}

	// Lost a stable projection (parallel / behind-camera). Drop the latch so the
	// next valid hit re-seeds instead of applying a jump from the last parameter.
	bInOutHasPreviousParameter = false;
	return DIVE::MapScreenDeltaToAxisTravel(
		Update.ScreenDelta,
		Update.ViewRotation,
		FrozenAxisWorld,
		CmPerPixel);
}
} // namespace

bool UDIVELinearDriveAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	UPrimitiveComponent* Target = Context.Target.Get();
	if (!Target)
	{
		return false;
	}

	IsolateDrivenPrimitive(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	ActiveTarget = Target;
	StartRelativeTransform = Target->GetRelativeTransform();
	FDrivenPrimitiveRest& RestState = RestStore.GetOrCapture(Target);
	RecaptureLinearRestIfReoriented(RestState, Target);
	RestRelativeTransform = RestState.RelativeTransform;
	AccumulatedTravelCm = SeedTravelAlongAxis(
		RestRelativeTransform,
		StartRelativeTransform,
		DriveAxisLocal(Axis));
	AccumulatedTravelCm = ClampToLimits(
		AccumulatedTravelCm,
		bLimitTravel,
		MinTravelCm,
		MaxTravelCm);
	ApplyAccumulated();
	InitializeLinearGestureState(
		Context,
		Target,
		Axis,
		FrozenAxisWorld,
		AxisOrigin,
		PreviousParameterCm,
		bHasPreviousParameter,
		bAwaitingLinearSeed);
	NotifyInteractionValue(MakeInteractionValue());
	return true;
}

void UDIVELinearDriveAction::UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update)
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target || !IsInteractionActive())
	{
		return;
	}

	const float DeltaCm = ResolveLinearDeltaCm(
		Update,
		AxisOrigin,
		FrozenAxisWorld,
		PreviousParameterCm,
		bHasPreviousParameter,
		bAwaitingLinearSeed,
		CmPerPixel);
	if (FMath::IsNearlyZero(DeltaCm))
	{
		return;
	}
	AccumulatedTravelCm = IntegrateAgainstLimits(
		AccumulatedTravelCm,
		DeltaCm,
		bLimitTravel,
		MinTravelCm,
		MaxTravelCm);
	ApplyAccumulated();
	NotifyInteractionValue(MakeInteractionValue());
}

void UDIVELinearDriveAction::EndInteraction_Implementation(const bool bCommit)
{
	if (UPrimitiveComponent* Target = ActiveTarget.Get())
	{
		if (!bCommit && bRestoreOnCancel)
		{
			SetDrivenRelativeTransform(Target, StartRelativeTransform);
		}
		RestoreDrivenPrimitiveIsolation(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	}

	bHasSavedIsolation = false;
	ActiveTarget.Reset();
	AccumulatedTravelCm = 0.f;
	ClearLinearGestureState(
		FrozenAxisWorld,
		AxisOrigin,
		PreviousParameterCm,
		bHasPreviousParameter,
		bAwaitingLinearSeed);
	NotifyInteractionCompleted();
}

void UDIVELinearDriveAction::ApplyAccumulated()
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target)
	{
		return;
	}

	const FVector AxisLocal = DriveAxisLocal(Axis);
	const FVector AxisInParent = RestRelativeTransform.TransformVectorNoScale(AxisLocal).GetSafeNormal();
	FTransform NewRel = StartRelativeTransform;
	NewRel.SetTranslation(RestRelativeTransform.GetTranslation() + AxisInParent * GetAppliedTravelCm());
	SetDrivenRelativeTransform(Target, NewRel);
}

float UDIVELinearDriveAction::GetAppliedTravelCm() const
{
	float Applied = AccumulatedTravelCm;
	if (DetentStepCm > KINDA_SMALL_NUMBER)
	{
		Applied = FMath::GridSnap(Applied, DetentStepCm);
	}
	if (bLimitTravel)
	{
		const float Lo = FMath::Min(MinTravelCm, MaxTravelCm);
		const float Hi = FMath::Max(MinTravelCm, MaxTravelCm);
		Applied = FMath::Clamp(Applied, Lo, Hi);
	}
	return Applied;
}

float UDIVELinearDriveAction::GetNormalizedValue() const
{
	if (!bLimitTravel)
	{
		// Unlimited: Normalized is not a position fraction. Consumers use Absolute (cm);
		// AbsoluteMax == 0 marks unbounded (HUD already prints Absolute cm).
		return 0.f;
	}

	const float Lo = FMath::Min(MinTravelCm, MaxTravelCm);
	const float Hi = FMath::Max(MinTravelCm, MaxTravelCm);
	const float Span = Hi - Lo;
	if (Span <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return (GetAppliedTravelCm() - Lo) / Span;
}

FDIVEInteractionValue UDIVELinearDriveAction::MakeInteractionValue() const
{
	FDIVEInteractionValue Value;
	Value.Normalized = GetNormalizedValue();
	if (bUseDomainReadout && bLimitTravel)
	{
		ApplyDomainReadout(Value, DomainMin, DomainMax, ReadoutSuffix);
	}
	else
	{
		Value.Absolute = GetAppliedTravelCm();
		Value.AbsoluteMax = bLimitTravel ? FMath::Abs(MaxTravelCm - MinTravelCm) : 0.f;
		Value.Unit = EDIVEInteractionValueUnit::Centimeters;
	}
	return Value;
}
