// Copyright (c) 2026. All Rights Reserved.

#include "DIVESessionSubsystem.h"

#include "Actions/DIVEBuiltInActions.h"
#include "DIVECameraRig.h"
#include "DIVEInspectableComponent.h"
#include "DIVELog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Session/DIVESessionFocusOps.h"
#include "Session/DIVESessionIsolationOps.h"
#include "Session/DIVESessionPhysicalDriveOps.h"
#include "Session/DIVESessionPickOps.h"
#include "Utils/DIVEContextMenu.h"
#include "Utils/DIVEPlayerQuery.h"

void UDIVESessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Ensure the session is terminated if the world tears down while a session is active (e.g.
	// during seamless level travel or PIE teardown started before EndPlay propagates).
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &UDIVESessionSubsystem::HandleWorldBeginTearDown);
}

void UDIVESessionSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
	EndSession(EDIVESessionEndReason::Forced);
	Super::Deinitialize();
}

bool UDIVESessionSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UDIVESessionSubsystem::HandleWorldBeginTearDown(UWorld* InWorld)
{
	// Only end if the tearing-down world is the one that hosts the current session.
	if (IsSessionActive() && InWorld && InWorld == GetWorld())
	{
		EndSession(EDIVESessionEndReason::Forced);
	}
}

bool UDIVESessionSubsystem::TryBeginSession(
	AActor* DeviceHost,
	UDIVEInspectableComponent* Inspectable,
	const FDIVESessionParams& Params)
{
	if (!DeviceHost || !Inspectable || !GetWorld())
	{
		return false;
	}

	UWorld* World = GetWorld();
	APlayerController* PlayerController = DIVEPlayerQuery::FindLocalPlayerController(World);
	if (!PlayerController)
	{
		return false;
	}

	if (SessionState == EDIVESessionState::Active)
	{
		EndSession(EDIVESessionEndReason::SessionRestart);
	}

	Inspectable->BuildSemanticRegistry();

	// Create the internal proxy-drive action that routes Physical-mode IDIVEProxyDrive hits through
	// the standard continuous-action slot, avoiding a parallel code path.
	if (!InternalProxyDriveAction)
	{
		InternalProxyDriveAction = NewObject<UDIVEProxyDriveForwardAction>(this, TEXT("InternalProxyDriveAction"));
	}

	ActiveDeviceHost = DeviceHost;
	ActiveInspectable = Inspectable;
	FocusStack.Reset();
	FocusStack.Add(FDIVEFocusTarget::MakeDeviceRoot());
	FocusedTarget = FDIVEFocusTarget::MakeDeviceRoot();
	InteractionMode = EDIVESessionInteractionMode::Default;

	if (AActor* CurrentViewTarget = PlayerController->GetViewTarget())
	{
		PreviousViewTarget = CurrentViewTarget;
	}

	FVector ViewLocation;
	FRotator ViewRotation;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	ADIVECameraRig* CameraRig = World->SpawnActor<ADIVECameraRig>(
		ADIVECameraRig::StaticClass(),
		ViewLocation,
		ViewRotation,
		SpawnParams);
	if (!CameraRig)
	{
		ActiveDeviceHost.Reset();
		ActiveInspectable.Reset();
		PreviousViewTarget.Reset();
		FocusStack.Reset();
		return false;
	}

	const FDIVECameraEffectiveSettings CameraSettings = Inspectable->GetEffectiveCameraSettings();
	SessionDefaultOrbitDistance = CameraSettings.DefaultOrbitDistance;

	ActiveCameraRig = CameraRig;
	ApplyCameraInputFromInspectable();

	constexpr bool bRefreshTransform = false;
	CameraRig->SetOrbitDistance(SessionDefaultOrbitDistance, bRefreshTransform);
	CameraRig->SetOrbitTarget(FocusStack.Last().GetPivotLocation(DeviceHost), bRefreshTransform);
	CameraRig->SyncOrbitFromCurrentView();

	SessionState = EDIVESessionState::Active;

	Inspectable->PreloadPickHoverOverlays();

	PlayerController->SetViewTargetWithBlend(CameraRig, 0.f);
	Inspectable->NotifySessionLifecycle(true);

	if (!FDIVESessionFocusOps::ApplyInitialSessionFocus(*this, Params.InitialFocusId))
	{
		FDIVESessionFocusOps::ApplyFocusTarget(*this, FDIVEFocusTarget::MakeDeviceRoot(), false, true, false);
	}

	OnSessionStarted.Broadcast(DeviceHost, Inspectable);

	return true;
}

void UDIVESessionSubsystem::EndSession(EDIVESessionEndReason Reason)
{
	if (SessionState == EDIVESessionState::Inactive)
	{
		return;
	}

	AActor* EndedDeviceHost = ActiveDeviceHost.Get();

	CloseContextMenu();
	FDIVESessionPickOps::ClearPickHover(*this);
	FDIVESessionPhysicalDriveOps::ClearProxyDrive(*this);
	FDIVESessionIsolationOps::ClearIsolation(*this);
	FocusStack.Reset();
	InteractionMode = EDIVESessionInteractionMode::Default;

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = DIVEPlayerQuery::FindLocalPlayerController(World))
		{
			AActor* RestoreTarget = PreviousViewTarget.Get();
			if (!RestoreTarget)
			{
				RestoreTarget = PlayerController->GetPawn();
			}

			if (RestoreTarget)
			{
				PlayerController->SetViewTarget(RestoreTarget);
			}
		}
	}

	if (UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get())
	{
		Inspectable->NotifySessionLifecycle(false);
	}

	if (ADIVECameraRig* CameraRig = ActiveCameraRig.Get())
	{
		CameraRig->Destroy();
	}

	ActiveCameraRig.Reset();
	ActiveDeviceHost.Reset();
	ActiveInspectable.Reset();
	PreviousViewTarget.Reset();
	FocusedTarget = FDIVEFocusTarget::MakeDeviceRoot();
	SessionState = EDIVESessionState::Inactive;

	OnSessionEnded.Broadcast(Reason, EndedDeviceHost);
}

void UDIVESessionSubsystem::ApplyOrbitInput(const FVector2D& Delta)
{
	if (!IsSessionActive())
	{
		return;
	}

	if (ADIVECameraRig* CameraRig = ActiveCameraRig.Get())
	{
		CameraRig->ApplyOrbitDelta(Delta);
	}
}

void UDIVESessionSubsystem::ApplyZoomInput(float Delta)
{
	if (!IsSessionActive())
	{
		return;
	}

	if (ADIVECameraRig* CameraRig = ActiveCameraRig.Get())
	{
		CameraRig->ApplyZoomDelta(Delta);
	}
}

void UDIVESessionSubsystem::ConfigureActiveCameraInput(float OrbitSensitivity, float ZoomSensitivity)
{
	if (ADIVECameraRig* CameraRig = ActiveCameraRig.Get())
	{
		CameraRig->SetInputSensitivity(OrbitSensitivity, ZoomSensitivity);
	}
}

void UDIVESessionSubsystem::ApplyCameraInputFromInspectable()
{
	if (UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get())
	{
		const FDIVECameraEffectiveSettings Settings = Inspectable->GetEffectiveCameraSettings();
		ConfigureActiveCameraInput(Settings.OrbitSensitivity, Settings.ZoomSensitivity);

		if (ADIVECameraRig* CameraRig = ActiveCameraRig.Get())
		{
			CameraRig->SetOrbitDistanceLimits(Settings.MinOrbitDistanceCm, Settings.MaxOrbitDistanceCm);
			CameraRig->SetZoomDistanceScaling(Settings.bScaleZoomWithOrbitDistance, Settings.ZoomDistanceReferenceCm);
			CameraRig->SetFocusClearanceRadius(Inspectable->ComputeFocusClearanceRadius(FocusedTarget));
		}
	}
}

bool UDIVESessionSubsystem::FocusAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController)
{
	FDIVEFocusTarget SelectedTarget;
	if (!ResolvePickAtScreenPosition(ScreenPosition, PlayerController, SelectedTarget))
	{
		return false;
	}

	return FocusTarget(SelectedTarget, true);
}

void UDIVESessionSubsystem::SetInteractionMode(EDIVESessionInteractionMode NewMode)
{
	if (InteractionMode == NewMode)
	{
		return;
	}

	CloseContextMenu();
	FDIVESessionPickOps::ClearPickHover(*this);

	if (NewMode != EDIVESessionInteractionMode::Physical)
	{
		FDIVESessionPhysicalDriveOps::ClearProxyDrive(*this);
	}

	InteractionMode = NewMode;
	OnInteractionModeChanged.Broadcast(NewMode);
}

bool UDIVESessionSubsystem::FocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack)
{
	if (!IsSessionActive() || !Target.IsValidFocus())
	{
		return false;
	}

	const bool bFitOrbitDistance = Target.Kind == EDIVEFocusKind::Primitive;
	return FDIVESessionFocusOps::ApplyFocusTarget(*this, Target, bPushToStack, true, bFitOrbitDistance);
}

bool UDIVESessionSubsystem::NavigateBack()
{
	if (!IsSessionActive())
	{
		return false;
	}

	if (FocusStack.Num() <= 1)
	{
		return false;
	}

	FocusStack.Pop();
	return FDIVESessionFocusOps::ApplyFocusTarget(*this, FocusStack.Last(), false, true, false);
}

bool UDIVESessionSubsystem::BuildContextMenuEntries(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	TArray<FDIVEContextMenuEntry>& OutEntries,
	FDIVEFocusTarget& OutPickTarget,
	FHitResult& OutPickHit) const
{
	OutEntries.Reset();
	OutPickTarget = FDIVEFocusTarget::MakeDeviceRoot();
	OutPickHit = FHitResult();

	if (!IsSessionActive() || !PlayerController)
	{
		return false;
	}

	const bool bHasValidPick = ResolvePickAtScreenPositionWithHit(
		ScreenPosition,
		PlayerController,
		OutPickTarget,
		OutPickHit);
	if (!bHasValidPick)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	DIVEContextMenu::BuildEntries(
		Inspectable,
		OutPickTarget,
		OutEntries);

	return !OutEntries.IsEmpty();
}

bool UDIVESessionSubsystem::OpenContextMenuAtScreenPosition(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	if (!IsSessionActive() || !PlayerController)
	{
		return false;
	}

	if (bContextMenuOpen)
	{
		CloseContextMenu();
		return false;
	}

	FDIVEFocusTarget PickTarget;
	FHitResult PickHit;
	if (!BuildContextMenuEntries(ScreenPosition, PlayerController, ContextMenuEntries, PickTarget, PickHit))
	{
		return false;
	}

	ContextMenuPickTarget = PickTarget;
	ContextMenuPickHit = PickHit;
	ContextMenuScreenPosition = ScreenPosition;
	bIgnoreNextPrimaryActionRelease = false;
	bContextMenuOpen = true;
	FDIVESessionPickOps::ClearPickHover(*this);
	OnContextMenuVisibilityChanged.Broadcast(true);
	return true;
}

bool UDIVESessionSubsystem::ExecuteContextMenuAction(
	UDIVEDeviceAction* Action,
	FName TargetKey,
	FName BindingId)
{
	if (!IsSessionActive() || !bContextMenuOpen || !Action)
	{
		return false;
	}

	const FDIVEFocusTarget PickTarget = ContextMenuPickTarget;
	const FVector2D ScreenPosition = ContextMenuScreenPosition;
	const FHitResult PickHit = ContextMenuPickHit;
	CloseContextMenu();

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	if (!Inspectable)
	{
		return false;
	}

	const FDIVEActionContext Context = Inspectable->MakeActionContext(
		PickTarget,
		TargetKey,
		ScreenPosition,
		PickHit,
		BindingId);

	return ExecuteResolvedAction(Action, Context, true);
}

bool UDIVESessionSubsystem::ExecuteResolvedAction(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context,
	const bool bSetIgnoreNextReleaseIfContinuous)
{
	if (!IsSessionActive() || !Action)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	if (!Inspectable)
	{
		return false;
	}

	FDIVEActionWorldScope WorldScope(Action, GetWorld());
	if (!Action->CanExecute(Context))
	{
		return false;
	}

	if (UDIVEContinuousDeviceAction* Continuous = Cast<UDIVEContinuousDeviceAction>(Action))
	{
		if (!TryBeginContinuousAction(Continuous, Context))
		{
			return false;
		}

		if (bSetIgnoreNextReleaseIfContinuous)
		{
			bIgnoreNextPrimaryActionRelease = true;
		}

		return true;
	}

	if (!Action->Execute(Context))
	{
		return false;
	}

	Inspectable->NotifyActionExecuted(Action, Context);
	return true;
}

void UDIVESessionSubsystem::CloseContextMenu()
{
	if (!bContextMenuOpen)
	{
		return;
	}

	bContextMenuOpen = false;
	ContextMenuEntries.Reset();
	ContextMenuPickTarget = FDIVEFocusTarget::MakeDeviceRoot();
	ContextMenuScreenPosition = FVector2D::ZeroVector;
	ContextMenuPickHit = FHitResult();
	OnContextMenuVisibilityChanged.Broadcast(false);
}

bool UDIVESessionSubsystem::ConsumeIgnoreNextPrimaryActionRelease()
{
	if (!bIgnoreNextPrimaryActionRelease)
	{
		return false;
	}

	bIgnoreNextPrimaryActionRelease = false;
	return true;
}

bool UDIVESessionSubsystem::IsIsolationActiveForTarget(const FDIVEFocusTarget& Target) const
{
	return bIsolationActive && IsolationTarget.Equals(Target);
}

bool UDIVESessionSubsystem::ToggleIsolateFocused()
{
	return ToggleIsolationForTarget(FocusedTarget);
}

bool UDIVESessionSubsystem::ToggleIsolationForTarget(const FDIVEFocusTarget& Target)
{
	if (!IsSessionActive())
	{
		return false;
	}

	if (bIsolationActive && IsolationTarget.Equals(Target))
	{
		FDIVESessionIsolationOps::ClearIsolation(*this);
		return true;
	}

	return FDIVESessionIsolationOps::ApplyIsolationForTarget(*this, Target);
}

bool UDIVESessionSubsystem::ToggleMeshPhysicsForTarget(const FDIVEFocusTarget& Target)
{
	if (!IsSessionActive() || Target.Kind != EDIVEFocusKind::Primitive)
	{
		return false;
	}

	UPrimitiveComponent* Primitive = Target.Primitive.Get();
	if (!Primitive)
	{
		return false;
	}

	const bool bEnablePhysics = !Primitive->IsSimulatingPhysics();
	Primitive->SetSimulatePhysics(bEnablePhysics);
	if (bEnablePhysics)
	{
		Primitive->WakeAllRigidBodies();
	}

	return true;
}

bool UDIVESessionSubsystem::DeleteMeshForTarget(const FDIVEFocusTarget& Target)
{
	if (!IsSessionActive() || Target.Kind != EDIVEFocusKind::Primitive)
	{
		return false;
	}

	UPrimitiveComponent* Primitive = Target.Primitive.Get();
	if (!Primitive || !Primitive->GetOwner())
	{
		return false;
	}

	if (FocusedTarget.Primitive.Get() == Primitive)
	{
		NavigateBack();
	}

	if (bIsolationActive)
	{
		FDIVESessionIsolationOps::ClearIsolation(*this);
	}

	Primitive->DestroyComponent();
	return true;
}

bool UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

void UDIVESessionSubsystem::ClearIsolation()
{
	FDIVESessionIsolationOps::ClearIsolation(*this);
}

bool UDIVESessionSubsystem::IsPawnPhysicalDriveActive() const
{
	return bProxyDriving && ActivePhysicalDriveKind == EDIVEActivePhysicalDriveKind::PawnBridge;
}

bool UDIVESessionSubsystem::TryBeginProxyDriveAtScreenPosition(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	return FDIVESessionPhysicalDriveOps::TryBeginProxyDriveAtScreenPosition(*this, ScreenPosition, PlayerController);
}

void UDIVESessionSubsystem::UpdateActiveInteraction(const FVector2D& ScreenDelta)
{
	FDIVESessionPhysicalDriveOps::UpdateActiveInteraction(*this, ScreenDelta);
}

void UDIVESessionSubsystem::EndProxyDrive(bool bCommit)
{
	FDIVESessionPhysicalDriveOps::EndProxyDrive(*this, bCommit);
}

bool UDIVESessionSubsystem::TryBeginContinuousAction(
	UDIVEContinuousDeviceAction* Action,
	const FDIVEActionContext& Context)
{
	if (!IsSessionActive() || bProxyDriving || !Action)
	{
		return false;
	}

	FDIVEActionWorldScope WorldScope(Action, GetWorld());
	if (!Action->CanExecute(Context) || !Action->BeginInteraction(Context))
	{
		return false;
	}

	Action->MarkInteractionActive();

	ActivePhysicalDriveKind = EDIVEActivePhysicalDriveKind::ContinuousAction;
	ActiveContinuousAction = Action;
	ActivePawnPhysicalDrive.Reset();
	bProxyDriving = true;

	Action->OnValueChanged.AddUniqueDynamic(this, &UDIVESessionSubsystem::HandleContinuousActionValueChanged);

	if (UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get())
	{
		Inspectable->NotifyActionExecuted(Action, Context);
	}

	return true;
}

void UDIVESessionSubsystem::HandleContinuousActionValueChanged(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context,
	float NormalizedValue)
{
	NotifyInteractionValueChanged(Action, Context, NormalizedValue);
}

void UDIVESessionSubsystem::NotifyInteractionValueChanged(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context,
	float NormalizedValue)
{
	OnInteractionValueChanged.Broadcast(Action, Context, NormalizedValue);
}

void UDIVESessionSubsystem::HandleActivePawnPhysicalManualRotatePressed()
{
	FDIVESessionPhysicalDriveOps::HandleActivePawnPhysicalManualRotatePressed(*this);
}

void UDIVESessionSubsystem::HandleActivePawnPhysicalManualRotateReleased()
{
	FDIVESessionPhysicalDriveOps::HandleActivePawnPhysicalManualRotateReleased(*this);
}

bool UDIVESessionSubsystem::ResolvePickAtScreenPositionWithHit(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	FDIVEFocusTarget& OutPickTarget,
	FHitResult& OutHit) const
{
	return FDIVESessionPickOps::ResolvePickAtScreenPositionWithHit(*this, ScreenPosition, PlayerController, OutPickTarget, OutHit);
}

bool UDIVESessionSubsystem::ResolvePickAtScreenPosition(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	FDIVEFocusTarget& OutPickTarget) const
{
	FHitResult UnusedHit;
	return ResolvePickAtScreenPositionWithHit(ScreenPosition, PlayerController, OutPickTarget, UnusedHit);
}

bool UDIVESessionSubsystem::ExecutePrimaryActionAtScreenPosition(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	return FDIVESessionPickOps::ExecutePrimaryActionAtScreenPosition(*this, ScreenPosition, PlayerController);
}

void UDIVESessionSubsystem::UpdatePickHover(const FVector2D& ScreenPosition, APlayerController* PlayerController)
{
	FDIVESessionPickOps::UpdatePickHover(*this, ScreenPosition, PlayerController);
}

void UDIVESessionSubsystem::ClearPickHover()
{
	FDIVESessionPickOps::ClearPickHover(*this);
}
