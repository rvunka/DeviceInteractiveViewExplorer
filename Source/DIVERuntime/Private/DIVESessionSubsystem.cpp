// Copyright (c) 2026. All Rights Reserved.

#include "DIVESessionSubsystem.h"

#include "DIVECameraRig.h"
#include "DIVEInspectableComponent.h"
#include "DIVELog.h"
#include "GameFramework/PlayerController.h"
#include "Session/DIVESessionFocusOps.h"
#include "Session/DIVESessionIsolationOps.h"
#include "Session/DIVESessionPhysicalDriveOps.h"
#include "Session/DIVESessionPickOps.h"
#include "Utils/DIVEContextMenu.h"
#include "Utils/DIVEPlayerQuery.h"

void UDIVESessionSubsystem::Deinitialize()
{
	EndSession(EDIVESessionEndReason::Forced);
	Super::Deinitialize();
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
	FDIVEFocusTarget& OutPickTarget) const
{
	OutEntries.Reset();
	OutPickTarget = FDIVEFocusTarget::MakeDeviceRoot();

	if (!IsSessionActive() || !PlayerController)
	{
		return false;
	}

	const bool bHasValidPick = ResolvePickAtScreenPosition(ScreenPosition, PlayerController, OutPickTarget);
	if (!bHasValidPick || OutPickTarget.Kind == EDIVEFocusKind::Anchor)
	{
		return false;
	}

	const UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	const bool bIncludeAdminMeshEntries = Inspectable && Inspectable->bEnableAdminContextMenuEntries;

	DIVEContextMenu::BuildStandardEntries(this, OutPickTarget, bHasValidPick, bIncludeAdminMeshEntries, OutEntries);
	DIVEContextMenu::AppendCustomEntries(ActiveDeviceHost.Get(), this, OutPickTarget, OutEntries);

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
	if (!BuildContextMenuEntries(ScreenPosition, PlayerController, ContextMenuEntries, PickTarget))
	{
		return false;
	}

	ContextMenuPickTarget = PickTarget;
	ContextMenuScreenPosition = ScreenPosition;
	bContextMenuOpen = true;
	FDIVESessionPickOps::ClearPickHover(*this);
	OnContextMenuVisibilityChanged.Broadcast(true);
	return true;
}

bool UDIVESessionSubsystem::ExecuteContextMenuAction(FName ActionId)
{
	if (!IsSessionActive() || !bContextMenuOpen || ActionId.IsNone())
	{
		return false;
	}

	const FDIVEFocusTarget PickTarget = ContextMenuPickTarget;
	CloseContextMenu();

	if (ActionId == DIVE::kContextFocus)
	{
		if (!PickTarget.IsValidFocus())
		{
			return false;
		}

		return FocusTarget(PickTarget, true);
	}

	if (ActionId == DIVE::kContextIsolate)
	{
		return ToggleIsolationForTarget(PickTarget);
	}

	if (ActionId == DIVE::kContextToggleMeshPhysics)
	{
		return AreAdminContextMenuEntriesAllowed() && ToggleMeshPhysicsForTarget(PickTarget);
	}

	if (ActionId == DIVE::kContextDeleteMesh)
	{
		return AreAdminContextMenuEntriesAllowed() && DeleteMeshForTarget(PickTarget);
	}

	if (UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get())
	{
		return Inspectable->NotifyPickContextMenuAction(ActionId, PickTarget);
	}

	return false;
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
	OnContextMenuVisibilityChanged.Broadcast(false);
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
	if (!AreAdminContextMenuEntriesAllowed() || !IsSessionActive() || Target.Kind != EDIVEFocusKind::Primitive)
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
	if (!AreAdminContextMenuEntriesAllowed() || !IsSessionActive() || Target.Kind != EDIVEFocusKind::Primitive)
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

bool UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed() const
{
#if UE_BUILD_SHIPPING
	return false;
#else
	const UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	return Inspectable && Inspectable->bEnableAdminContextMenuEntries;
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

void UDIVESessionSubsystem::UpdateProxyDrive(const FVector2D& ScreenDelta)
{
	FDIVESessionPhysicalDriveOps::UpdateProxyDrive(*this, ScreenDelta);
}

void UDIVESessionSubsystem::EndProxyDrive(bool bCommit)
{
	FDIVESessionPhysicalDriveOps::EndProxyDrive(*this, bCommit);
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
