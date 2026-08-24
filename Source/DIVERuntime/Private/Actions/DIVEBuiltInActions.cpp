// Copyright (c) 2026. All Rights Reserved.

#include "Actions/DIVEBuiltInActions.h"

#include "Components/PrimitiveComponent.h"
#include "Templates/TypeHash.h"
#include "DIVEDriveMapping.h"
#include "DIVEProxyDrive.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVESessionSubsystem.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"

namespace
{
UDIVESessionSubsystem* ResolveSessionSubsystem(const UObject* WorldContext, const FDIVEActionContext& Context)
{
	// Try the device host first — it is always world-bound, even when the action itself is
	// instanced in a UDIVEActionCatalogAsset (where WorldContext->GetWorld() returns nullptr).
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
} // namespace

UDIVEFocusAction::UDIVEFocusAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuFocus", "Focus");
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

UDIVEProxyDriveForwardAction::UDIVEProxyDriveForwardAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ProxyDriveForward", "Drive");
}

bool UDIVEProxyDriveForwardAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context) || !Context.Target)
	{
		return false;
	}

	if (IDIVEProxyDrive* ProxyDrive = DIVEProxyDriveResolve::FindProxyDriveForHit(Context.Target))
	{
		if (UObject* ProxyObject = Cast<UObject>(ProxyDrive))
		{
			FDIVEProxyDriveContext DriveContext;
			DriveContext.ScreenPosition = Context.ScreenPosition;
			DriveContext.FocusTarget = Context.PickTarget;
			DriveContext.HitComponent = Context.Target;
			DriveContext.PickHit = Context.PickHit;
			DriveContext.ViewLocation = Context.ViewLocation;
			DriveContext.ViewRotation = Context.ViewRotation;
			DriveContext.PickRayDir = Context.PickRayDir;
			return IDIVEProxyDrive::Execute_CanProxyDrive(ProxyObject, DriveContext);
		}
	}
	return false;
}

bool UDIVEProxyDriveForwardAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	ActiveProxyObject.Reset();
	if (!Context.Target)
	{
		return false;
	}

	IDIVEProxyDrive* ProxyDrive = DIVEProxyDriveResolve::FindProxyDriveForHit(Context.Target);
	UObject* ProxyObject = Cast<UObject>(ProxyDrive);
	FDIVEProxyDriveContext DriveContext;
	DriveContext.ScreenPosition = Context.ScreenPosition;
	DriveContext.FocusTarget = Context.PickTarget;
	DriveContext.HitComponent = Context.Target;
	DriveContext.PickHit = Context.PickHit;
	DriveContext.ViewLocation = Context.ViewLocation;
	DriveContext.ViewRotation = Context.ViewRotation;
	DriveContext.PickRayDir = Context.PickRayDir;
	if (!ProxyObject || !IDIVEProxyDrive::Execute_CanProxyDrive(ProxyObject, DriveContext))
	{
		return false;
	}

	if (!IDIVEProxyDrive::Execute_BeginProxyDrive(ProxyObject, DriveContext))
	{
		return false;
	}

	ActiveProxyObject = ProxyObject;

	float NormalizedValue = 0.f;
	if (IDIVEProxyDrive::Execute_GetProxyDriveNormalizedValue(ProxyObject, NormalizedValue))
	{
		NotifyValueChanged(NormalizedValue);
	}

	return true;
}

void UDIVEProxyDriveForwardAction::UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update)
{
	UObject* ProxyObject = ActiveProxyObject.Get();
	if (!ProxyObject || Update.ScreenDelta.IsNearlyZero())
	{
		return;
	}

	IDIVEProxyDrive::Execute_ApplyProxyDriveDelta(ProxyObject, Update.ScreenDelta);

	float NormalizedValue = 0.f;
	if (IDIVEProxyDrive::Execute_GetProxyDriveNormalizedValue(ProxyObject, NormalizedValue))
	{
		NotifyValueChanged(NormalizedValue);
	}
}

void UDIVEProxyDriveForwardAction::EndInteraction_Implementation(bool bCommit)
{
	if (UObject* ProxyObject = ActiveProxyObject.Get())
	{
		IDIVEProxyDrive::Execute_EndProxyDrive(ProxyObject, bCommit);
	}
	ActiveProxyObject.Reset();
	NotifyInteractionCompleted();
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

struct FDrivenRestKey
{
	TWeakObjectPtr<const UPrimitiveComponent> Primitive;
	TWeakObjectPtr<const UClass> ActionClass;

	bool operator==(const FDrivenRestKey& Other) const
	{
		return Primitive == Other.Primitive && ActionClass == Other.ActionClass;
	}

	friend uint32 GetTypeHash(const FDrivenRestKey& Key)
	{
		return HashCombine(GetTypeHash(Key.Primitive.Get()), GetTypeHash(Key.ActionClass.Get()));
	}
};

struct FDrivenPrimitiveRest
{
	FTransform RelativeTransform = FTransform::Identity;
	float CommittedAngleDegrees = 0.f;
	float CommittedTurns = 0.f;
};

TMap<FDrivenRestKey, FDrivenPrimitiveRest> GDrivenPrimitiveRests;

void CompactDrivenPrimitiveRests()
{
	for (auto It = GDrivenPrimitiveRests.CreateIterator(); It; ++It)
	{
		if (!It.Key().Primitive.IsValid() || !It.Key().ActionClass.IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

FDrivenPrimitiveRest& GetOrCaptureDrivenRest(const UPrimitiveComponent* Target, const UClass* ActionClass)
{
	CompactDrivenPrimitiveRests();
	const FDrivenRestKey Key{Target, ActionClass};
	if (FDrivenPrimitiveRest* Found = GDrivenPrimitiveRests.Find(Key))
	{
		return *Found;
	}

	FDrivenPrimitiveRest Added;
	Added.RelativeTransform = Target->GetRelativeTransform();
	return GDrivenPrimitiveRests.Add(Key, MoveTemp(Added));
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

// Rotary never recaptures: angle-from-rest plus unwrap is the state.
// Linear recaptures only when the rail orientation was reauthored (rotation left rest).
// Threaded recaptures only when the pose left the screw (rotation not around Axis, or slide not along Axis).
// A full-transform Equals here is unsafe: ±180 is one pose with two Euler writings, and a false
// recapture would reset rest to the stop — the original infinite-spin bug.
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

FDrivenPrimitiveRest* FindDrivenRest(const UPrimitiveComponent* Target, const UClass* ActionClass)
{
	if (!Target || !ActionClass)
	{
		return nullptr;
	}
	return GDrivenPrimitiveRests.Find(FDrivenRestKey{Target, ActionClass});
}

void CommitDrivenAngle(const UPrimitiveComponent* Target, const UClass* ActionClass, const float AngleDegrees)
{
	if (FDrivenPrimitiveRest* Found = FindDrivenRest(Target, ActionClass))
	{
		Found->CommittedAngleDegrees = AngleDegrees;
	}
}

void CommitDrivenTurns(const UPrimitiveComponent* Target, const UClass* ActionClass, const float Turns)
{
	if (FDrivenPrimitiveRest* Found = FindDrivenRest(Target, ActionClass))
	{
		Found->CommittedTurns = Turns;
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

UDIVERotaryDriveAction::UDIVERotaryDriveAction()
{
	DisplayName = NSLOCTEXT("DIVE", "RotaryDrive", "Rotate");
}

bool UDIVERotaryDriveAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	return Super::CanExecute_Implementation(Context)
		&& Context.Target != nullptr;
}

bool UDIVERotaryDriveAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	UPrimitiveComponent* Target = Context.Target.Get();
	if (!Target)
	{
		return false;
	}

	IsolateDrivenPrimitive(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	ActiveTarget = Target;
	StartRelativeRotation = Target->GetRelativeRotation();
	FDrivenPrimitiveRest& RestState = GetOrCaptureDrivenRest(Target, GetClass());
	RestRelativeRotation = RestState.RelativeTransform.GetRotation();
	AccumulatedDegrees = SeedUnwrappedDegrees(
		SignedDegreesAroundAxis(
			RestRelativeRotation,
			Target->GetRelativeRotation().Quaternion(),
			DriveAxisLocal(Axis)),
		RestState.CommittedAngleDegrees);
	ApplyAccumulated();
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
	NotifyInteractionValue(MakeInteractionValue());
	return true;
}

void UDIVERotaryDriveAction::UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update)
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target || !IsInteractionActive())
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
	AccumulatedDegrees += DeltaDeg;
	ApplyAccumulated();
	NotifyInteractionValue(MakeInteractionValue());
}

void UDIVERotaryDriveAction::EndInteraction_Implementation(const bool bCommit)
{
	if (UPrimitiveComponent* Target = ActiveTarget.Get())
	{
		if (bCommit)
		{
			CommitDrivenAngle(Target, GetClass(), GetAppliedDegrees());
		}
		else if (bRestoreOnCancel)
		{
			SetDrivenRelativeRotation(Target, FQuat(StartRelativeRotation));
		}
		RestoreDrivenPrimitiveIsolation(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	}

	bHasSavedIsolation = false;
	ActiveTarget.Reset();
	AccumulatedDegrees = 0.f;
	ClearPolarGestureState(
		FrozenAxisWorld,
		AxisOrigin,
		PlaneBasisU,
		PlaneBasisV,
		PreviousPlaneVector,
		bAwaitingPolarSeed);
	NotifyInteractionCompleted();
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
}

bool UDIVEThreadedDriveAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	return Super::CanExecute_Implementation(Context)
		&& Context.Target != nullptr;
}

bool UDIVEThreadedDriveAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	UPrimitiveComponent* Target = Context.Target.Get();
	if (!Target)
	{
		return false;
	}

	IsolateDrivenPrimitive(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	ActiveTarget = Target;
	StartRelativeTransform = Target->GetRelativeTransform();
	FDrivenPrimitiveRest& RestState = GetOrCaptureDrivenRest(Target, GetClass());
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
	bReleased = false;
	ApplyAccumulated();
	if (GetAppliedTurns() >= TurnsToRelease - KINDA_SMALL_NUMBER)
	{
		ReleaseTarget();
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
	NotifyInteractionValue(MakeInteractionValue());
	return true;
}

void UDIVEThreadedDriveAction::UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update)
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target || !IsInteractionActive() || bReleased)
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
	const float SignedTurns = (DeltaDeg / 360.f) * (bPositiveDeltaLoosens ? 1.f : -1.f);
	AccumulatedTurns += SignedTurns;

	ApplyAccumulated();
	NotifyInteractionValue(MakeInteractionValue());

	if (GetAppliedTurns() >= TurnsToRelease - KINDA_SMALL_NUMBER)
	{
		ReleaseTarget();
	}
}

void UDIVEThreadedDriveAction::EndInteraction_Implementation(const bool bCommit)
{
	if (UPrimitiveComponent* Target = ActiveTarget.Get())
	{
		if (bCommit)
		{
			CommitDrivenTurns(Target, GetClass(), GetAppliedTurns());
		}
		else if (!bReleased)
		{
			SetDrivenRelativeTransform(Target, StartRelativeTransform);
		}
		if (!bReleased)
		{
			RestoreDrivenPrimitiveIsolation(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
		}
	}

	bHasSavedIsolation = false;
	ActiveTarget.Reset();
	AccumulatedTurns = 0.f;
	bReleased = false;
	ClearPolarGestureState(
		FrozenAxisWorld,
		AxisOrigin,
		PlaneBasisU,
		PlaneBasisV,
		PreviousPlaneVector,
		bAwaitingPolarSeed);
	NotifyInteractionCompleted();
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
	FDrivenPrimitiveRest& RestState = GetOrCaptureDrivenRest(Target, GetClass());
	RecaptureLinearRestIfReoriented(RestState, Target);
	RestRelativeTransform = RestState.RelativeTransform;
	AccumulatedTravelCm = SeedTravelAlongAxis(
		RestRelativeTransform,
		StartRelativeTransform,
		DriveAxisLocal(Axis));
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
	AccumulatedTravelCm += DeltaCm;
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
