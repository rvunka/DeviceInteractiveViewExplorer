// Copyright (c) 2026. All Rights Reserved.

#include "Actions/DIVEBuiltInActions.h"

#include "Components/PrimitiveComponent.h"
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

void UDIVEProxyDriveForwardAction::UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime)
{
	(void)DeltaTime;
	UObject* ProxyObject = ActiveProxyObject.Get();
	if (!ProxyObject)
	{
		return;
	}

	IDIVEProxyDrive::Execute_ApplyProxyDriveDelta(ProxyObject, ScreenDelta);

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
	ViewRotation = Context.ViewRotation;
	FrozenAxisWorld = DriveAxisWorld(Target, Axis);
	AccumulatedDegrees = 0.f;
	NotifyInteractionValue(MakeInteractionValue());
	return true;
}

void UDIVERotaryDriveAction::UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime)
{
	(void)DeltaTime;
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target || !IsInteractionActive())
	{
		return;
	}

	const float DeltaDeg = DIVE::MapScreenDeltaToAxisAngle(
		ScreenDelta,
		ViewRotation,
		FrozenAxisWorld,
		DegreesPerPixel);
	AccumulatedDegrees += DeltaDeg;

	if (DetentStepDegrees > KINDA_SMALL_NUMBER)
	{
		AccumulatedDegrees = FMath::GridSnap(AccumulatedDegrees, DetentStepDegrees);
	}

	if (bLimitAngle)
	{
		const float Lo = FMath::Min(MinAngleDegrees, MaxAngleDegrees);
		const float Hi = FMath::Max(MinAngleDegrees, MaxAngleDegrees);
		AccumulatedDegrees = FMath::Clamp(AccumulatedDegrees, Lo, Hi);
	}

	ApplyAccumulated();
	NotifyInteractionValue(MakeInteractionValue());
}

void UDIVERotaryDriveAction::EndInteraction_Implementation(const bool bCommit)
{
	if (UPrimitiveComponent* Target = ActiveTarget.Get())
	{
		if (!bCommit && bRestoreOnCancel)
		{
			SetDrivenRelativeRotation(Target, FQuat(StartRelativeRotation));
		}
		RestoreDrivenPrimitiveIsolation(Target, SavedCollisionEnabled, bSavedAutoWeld, bHasSavedIsolation);
	}

	bHasSavedIsolation = false;
	ActiveTarget.Reset();
	AccumulatedDegrees = 0.f;
	NotifyInteractionCompleted();
}

void UDIVERotaryDriveAction::ApplyAccumulated()
{
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target)
	{
		return;
	}

	const FQuat DeltaQ(DriveAxisLocal(Axis), FMath::DegreesToRadians(AccumulatedDegrees));
	const FQuat NewRelQuat = FQuat(StartRelativeRotation) * DeltaQ;
	SetDrivenRelativeRotation(Target, NewRelQuat);
}

float UDIVERotaryDriveAction::GetNormalizedValue() const
{
	if (!bLimitAngle)
	{
		return FMath::Fmod(FMath::Abs(AccumulatedDegrees), 360.f) / 360.f;
	}

	const float Lo = FMath::Min(MinAngleDegrees, MaxAngleDegrees);
	const float Hi = FMath::Max(MinAngleDegrees, MaxAngleDegrees);
	const float Span = Hi - Lo;
	if (Span <= KINDA_SMALL_NUMBER)
	{
		return 0.f;
	}
	return (AccumulatedDegrees - Lo) / Span;
}

FDIVEInteractionValue UDIVERotaryDriveAction::MakeInteractionValue() const
{
	FDIVEInteractionValue Value;
	Value.Normalized = GetNormalizedValue();
	Value.Absolute = AccumulatedDegrees;
	Value.AbsoluteMax = 0.f;
	Value.Unit = EDIVEInteractionValueUnit::Degrees;
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
	ViewRotation = Context.ViewRotation;
	FrozenAxisWorld = DriveAxisWorld(Target, Axis);
	AccumulatedTurns = 0.f;
	bReleased = false;
	NotifyInteractionValue(MakeInteractionValue());
	return true;
}

void UDIVEThreadedDriveAction::UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime)
{
	(void)DeltaTime;
	UPrimitiveComponent* Target = ActiveTarget.Get();
	if (!Target || !IsInteractionActive() || bReleased)
	{
		return;
	}

	const float DeltaDeg = DIVE::MapScreenDeltaToAxisAngle(
		ScreenDelta,
		ViewRotation,
		FrozenAxisWorld,
		DegreesPerPixel);
	const float SignedTurns = (DeltaDeg / 360.f) * (bPositiveDeltaLoosens ? 1.f : -1.f);
	AccumulatedTurns = FMath::Clamp(AccumulatedTurns + SignedTurns, 0.f, TurnsToRelease);

	ApplyAccumulated();
	NotifyInteractionValue(MakeInteractionValue());

	if (AccumulatedTurns >= TurnsToRelease - KINDA_SMALL_NUMBER)
	{
		ReleaseTarget();
	}
}

void UDIVEThreadedDriveAction::EndInteraction_Implementation(const bool bCommit)
{
	if (UPrimitiveComponent* Target = ActiveTarget.Get())
	{
		if (!bCommit && !bReleased)
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
	const FQuat DeltaQ(AxisLocal, FMath::DegreesToRadians(AccumulatedTurns * 360.f));
	const FVector AxisInParent = StartRelativeTransform.TransformVectorNoScale(AxisLocal).GetSafeNormal();
	const FVector RelOffset = AxisInParent * (AccumulatedTurns * PitchCmPerTurn);

	FTransform NewRel = StartRelativeTransform;
	NewRel.SetRotation(StartRelativeTransform.GetRotation() * DeltaQ);
	NewRel.SetTranslation(StartRelativeTransform.GetTranslation() + RelOffset);
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

float UDIVEThreadedDriveAction::GetNormalizedValue() const
{
	if (TurnsToRelease <= KINDA_SMALL_NUMBER)
	{
		return 1.f;
	}
	return FMath::Clamp(AccumulatedTurns / TurnsToRelease, 0.f, 1.f);
}

FDIVEInteractionValue UDIVEThreadedDriveAction::MakeInteractionValue() const
{
	FDIVEInteractionValue Value;
	Value.Normalized = GetNormalizedValue();
	Value.Absolute = AccumulatedTurns;
	Value.AbsoluteMax = TurnsToRelease;
	Value.Unit = EDIVEInteractionValueUnit::Turns;
	return Value;
}
