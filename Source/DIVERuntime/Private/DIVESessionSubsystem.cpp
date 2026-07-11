// Copyright (c) 2026. All Rights Reserved.

#include "DIVESessionSubsystem.h"

#include "DIVEAnchorComponent.h"
#include "DIVECameraRig.h"
#include "DIVEHierarchy.h"
#include "DIVEInspectableComponent.h"
#include "DIVEProxyDrive.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVEProxyDriveTypes.h"
#include "Components/MeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Utils/DIVEPick.h"
#include "Utils/DIVEPlayerQuery.h"
#include "Utils/DIVEContextMenu.h"
#include "Containers/Set.h"

namespace
{
void SetMeshOverlayMaterial(UPrimitiveComponent* Primitive, UMaterialInterface* OverlayMaterial)
{
	if (UMeshComponent* Mesh = Cast<UMeshComponent>(Primitive))
	{
		Mesh->SetOverlayMaterial(OverlayMaterial);
	}
}
}

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

	const float DefaultDistance = Inspectable->GetEffectiveDefaultOrbitDistance();
	SessionDefaultOrbitDistance = DefaultDistance;

	constexpr bool bRefreshTransform = false;
	CameraRig->SetOrbitDistance(DefaultDistance, bRefreshTransform);
	CameraRig->SetOrbitTarget(FocusStack.Last().GetPivotLocation(DeviceHost), bRefreshTransform);
	CameraRig->SyncOrbitFromCurrentView();

	ApplyCameraInputFromInspectable();
	ActiveCameraRig = CameraRig;
	SessionState = EDIVESessionState::Active;

	PlayerController->SetViewTargetWithBlend(CameraRig, 0.f);
	Inspectable->NotifySessionLifecycle(true);

	if (!ApplyInitialSessionFocus(Params.InitialFocusId))
	{
		ApplyFocusTarget(FDIVEFocusTarget::MakeDeviceRoot(), false, true);
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
	ClearPickHover();
	ClearProxyDrive();
	ClearIsolation();
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
		ConfigureActiveCameraInput(
			Inspectable->GetEffectiveOrbitSensitivity(),
			Inspectable->GetEffectiveZoomSensitivity());
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
	ClearPickHover();

	if (NewMode != EDIVESessionInteractionMode::Physical)
	{
		ClearProxyDrive();
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

	return ApplyFocusTarget(Target, bPushToStack);
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
	return ApplyFocusTarget(FocusStack.Last(), false);
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

	DIVEContextMenu::BuildStandardEntries(this, OutPickTarget, bHasValidPick, OutEntries);
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
	ClearPickHover();
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
		return ToggleMeshPhysicsForTarget(PickTarget);
	}

	if (ActionId == DIVE::kContextDeleteMesh)
	{
		return DeleteMeshForTarget(PickTarget);
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
		ClearIsolation();
		return true;
	}

	return ApplyIsolationForTarget(Target);
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
		ClearIsolation();
	}

	Primitive->DestroyComponent();
	return true;
}

void UDIVESessionSubsystem::ClearIsolation()
{
	if (!bIsolationActive)
	{
		return;
	}

	ClearPickHover();

	for (const TWeakObjectPtr<UPrimitiveComponent>& WeakPrimitive : IsolatedHiddenPrimitives)
	{
		if (UPrimitiveComponent* Primitive = WeakPrimitive.Get())
		{
			Primitive->SetHiddenInGame(false);
		}
	}

	IsolatedHiddenPrimitives.Reset();
	bIsolationActive = false;
	IsolationTarget = FDIVEFocusTarget::MakeDeviceRoot();
}

void UDIVESessionSubsystem::EndActivePhysicalDrive(bool bCommit)
{
	switch (ActivePhysicalDriveKind)
	{
	case EDIVEActivePhysicalDriveKind::DeviceProxy:
		if (UObject* ProxyObject = ActiveProxyDrive.GetObject())
		{
			IDIVEProxyDrive::Execute_EndProxyDrive(ProxyObject, bCommit);
		}
		break;
	case EDIVEActivePhysicalDriveKind::PawnBridge:
		if (UObject* PawnDriveObject = ActivePawnPhysicalDrive.GetObject())
		{
			IDIVEPawnPhysicalDrive::Execute_EndPawnPhysicalDrive(PawnDriveObject, bCommit);
		}
		break;
	default:
		break;
	}
}

void UDIVESessionSubsystem::ResetPhysicalDriveState()
{
	bProxyDriving = false;
	ActivePhysicalDriveKind = EDIVEActivePhysicalDriveKind::None;
	ActiveProxyDrive.Reset();
	ActivePawnPhysicalDrive.Reset();
}

bool UDIVESessionSubsystem::IsPawnPhysicalDriveActive() const
{
	return bProxyDriving && ActivePhysicalDriveKind == EDIVEActivePhysicalDriveKind::PawnBridge;
}

void UDIVESessionSubsystem::ClearProxyDrive()
{
	if (!bProxyDriving)
	{
		return;
	}

	EndActivePhysicalDrive(false);
	ResetPhysicalDriveState();
}

bool UDIVESessionSubsystem::TryBeginProxyDriveAtScreenPosition(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	if (!IsSessionActive() || bProxyDriving || !PlayerController)
	{
		return false;
	}

	if (InteractionMode != EDIVESessionInteractionMode::Physical)
	{
		return false;
	}

	AActor* DeviceHost = ActiveDeviceHost.Get();
	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	UWorld* World = GetWorld();
	if (!DeviceHost || !Inspectable || !World)
	{
		return false;
	}

	FHitResult HitResult;
	FDIVEFocusTarget PickTarget;
	DIVEPick::FSessionPickContext Context;
	Context.World = World;
	Context.DeviceHost = DeviceHost;
	Context.Inspectable = Inspectable;
	Context.IgnoredActor = ActiveCameraRig.Get();
	Context.TraceChannel = Inspectable->PickTraceChannel;

	if (!DIVEPick::PickAtScreenPosition(Context, ScreenPosition, PlayerController, HitResult, PickTarget))
	{
		return false;
	}

	UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	if (!HitComponent)
	{
		return false;
	}

	FDIVEProxyDriveContext DriveContext;
	DriveContext.ScreenPosition = ScreenPosition;
	DriveContext.FocusTarget = PickTarget;
	DriveContext.HitComponent = HitComponent;
	DriveContext.PickHit = HitResult;

	if (IDIVEProxyDrive* ProxyDrive = DIVEProxyDriveResolve::FindProxyDriveForHit(HitComponent))
	{
		if (UObject* ProxyObject = Cast<UObject>(ProxyDrive))
		{
			if (IDIVEProxyDrive::Execute_CanProxyDrive(ProxyObject)
				&& IDIVEProxyDrive::Execute_BeginProxyDrive(ProxyObject, DriveContext))
			{
				ActivePhysicalDriveKind = EDIVEActivePhysicalDriveKind::DeviceProxy;
				ActiveProxyDrive = ProxyObject;
				ActivePawnPhysicalDrive.Reset();
				bProxyDriving = true;
				return true;
			}
		}
	}

	if (IDIVEPawnPhysicalDrive* PawnDrive = DIVEPawnPhysicalDriveResolve::FindOnPlayerController(PlayerController))
	{
		if (UObject* PawnDriveObject = Cast<UObject>(PawnDrive))
		{
			if (IDIVEPawnPhysicalDrive::Execute_CanBeginPawnPhysicalDrive(PawnDriveObject, DriveContext)
				&& IDIVEPawnPhysicalDrive::Execute_BeginPawnPhysicalDrive(PawnDriveObject, DriveContext))
			{
				ActivePhysicalDriveKind = EDIVEActivePhysicalDriveKind::PawnBridge;
				ActivePawnPhysicalDrive = PawnDriveObject;
				ActiveProxyDrive.Reset();
				bProxyDriving = true;
				return true;
			}
		}
	}

	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("DIVE: Physical pick on '%s' had no device proxy drive and no pawn physical drive backend."),
		*GetNameSafe(HitComponent));

	return false;
}

void UDIVESessionSubsystem::UpdateProxyDrive(const FVector2D& ScreenDelta)
{
	if (!bProxyDriving)
	{
		return;
	}

	switch (ActivePhysicalDriveKind)
	{
	case EDIVEActivePhysicalDriveKind::DeviceProxy:
		if (UObject* ProxyObject = ActiveProxyDrive.GetObject())
		{
			IDIVEProxyDrive::Execute_ApplyProxyDriveDelta(ProxyObject, ScreenDelta);
		}
		else
		{
			ClearProxyDrive();
		}
		break;
	case EDIVEActivePhysicalDriveKind::PawnBridge:
		if (UObject* PawnDriveObject = ActivePawnPhysicalDrive.GetObject())
		{
			IDIVEPawnPhysicalDrive::Execute_ApplyPawnPhysicalDriveDelta(PawnDriveObject, ScreenDelta);
		}
		else
		{
			ClearProxyDrive();
		}
		break;
	default:
		ClearProxyDrive();
		break;
	}
}

void UDIVESessionSubsystem::EndProxyDrive(bool bCommit)
{
	if (!bProxyDriving)
	{
		return;
	}

	EndActivePhysicalDrive(bCommit);
	ResetPhysicalDriveState();
}

void UDIVESessionSubsystem::HandleActivePawnPhysicalManualRotatePressed()
{
	if (!bProxyDriving || ActivePhysicalDriveKind != EDIVEActivePhysicalDriveKind::PawnBridge)
	{
		return;
	}

	if (UObject* PawnDriveObject = ActivePawnPhysicalDrive.GetObject())
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotatePressed(PawnDriveObject);
	}
}

void UDIVESessionSubsystem::HandleActivePawnPhysicalManualRotateReleased()
{
	if (!bProxyDriving || ActivePhysicalDriveKind != EDIVEActivePhysicalDriveKind::PawnBridge)
	{
		return;
	}

	if (UObject* PawnDriveObject = ActivePawnPhysicalDrive.GetObject())
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotateReleased(PawnDriveObject);
	}
}

bool UDIVESessionSubsystem::ResolvePickAtScreenPosition(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController,
	FDIVEFocusTarget& OutTarget) const
{
	OutTarget = FDIVEFocusTarget::MakeDeviceRoot();

	if (!IsSessionActive() || !PlayerController || !GetWorld())
	{
		return false;
	}

	AActor* DeviceHost = ActiveDeviceHost.Get();
	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	if (!DeviceHost || !Inspectable)
	{
		return false;
	}

	DIVEPick::FSessionPickContext Context;
	Context.World = GetWorld();
	Context.DeviceHost = DeviceHost;
	Context.Inspectable = Inspectable;
	Context.IgnoredActor = ActiveCameraRig.Get();
	Context.TraceChannel = Inspectable->PickTraceChannel;

	FHitResult HitResult;
	return DIVEPick::PickAtScreenPosition(Context, ScreenPosition, PlayerController, HitResult, OutTarget);
}

bool UDIVESessionSubsystem::ExecutePrimaryActionAtScreenPosition(
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	if (!IsSessionActive() || InteractionMode != EDIVESessionInteractionMode::Default || !PlayerController)
	{
		return false;
	}

	FDIVEFocusTarget PickTarget;
	if (!ResolvePickAtScreenPosition(ScreenPosition, PlayerController, PickTarget)
		|| PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	if (!Inspectable)
	{
		return false;
	}

	FName QualifiedActionId = NAME_None;
	if (!Inspectable->TryResolvePrimaryPickAction(PickTarget, QualifiedActionId))
	{
		return false;
	}

	return Inspectable->NotifyPickContextMenuAction(QualifiedActionId, PickTarget);
}

void UDIVESessionSubsystem::UpdatePickHover(const FVector2D& ScreenPosition, APlayerController* PlayerController)
{
	if (!IsSessionActive()
		|| InteractionMode != EDIVESessionInteractionMode::Default
		|| bContextMenuOpen
		|| !PlayerController)
	{
		ClearPickHover();
		return;
	}

	FDIVEFocusTarget PickTarget;
	if (!ResolvePickAtScreenPosition(ScreenPosition, PlayerController, PickTarget)
		|| PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		ClearPickHover();
		return;
	}

	UPrimitiveComponent* Primitive = PickTarget.Primitive.Get();
	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	if (!Primitive || !Inspectable || Primitive->bHiddenInGame || !Inspectable->IsPrimitiveInteractive(Primitive))
	{
		ClearPickHover();
		return;
	}

	UMaterialInterface* OverlayMaterial = Inspectable->ResolvePickHoverOverlayMaterial(PickTarget);
	if (!OverlayMaterial)
	{
		ClearPickHover();
		return;
	}

	if (PickHoverPrimitive.Get() == Primitive)
	{
		return;
	}

	ClearPickHover();
	SetMeshOverlayMaterial(Primitive, OverlayMaterial);
	PickHoverPrimitive = Primitive;
}

void UDIVESessionSubsystem::ClearPickHover()
{
	if (UPrimitiveComponent* Primitive = PickHoverPrimitive.Get())
	{
		SetMeshOverlayMaterial(Primitive, nullptr);
	}

	PickHoverPrimitive.Reset();
}

bool UDIVESessionSubsystem::ApplyFocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack, bool bBlendCamera, bool bUseDefaultOrbitDistance)
{
	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	ADIVECameraRig* CameraRig = ActiveCameraRig.Get();
	if (!Inspectable || !CameraRig)
	{
		return false;
	}

	if (bPushToStack)
	{
		if (FocusStack.IsEmpty() || !FocusStack.Last().Equals(Target))
		{
			FocusStack.Add(Target);
		}
	}
	else if (FocusStack.IsEmpty())
	{
		FocusStack.Add(Target);
	}
	else
	{
		FocusStack.Last() = Target;
	}

	FocusedTarget = Target;

	constexpr bool bRefreshTransform = false;
	if (FocusedTarget.Kind == EDIVEFocusKind::Anchor)
	{
		if (UDIVEAnchorComponent* Anchor = Cast<UDIVEAnchorComponent>(FocusedTarget.Anchor.Get()))
		{
			CameraRig->SetAnchorViewpoint(Anchor->GetComponentLocation(), Anchor->GetViewRotation(), bRefreshTransform);
		}
		else
		{
			CameraRig->SetOrbitTarget(FocusedTarget.GetPivotLocation(ActiveDeviceHost.Get()), bRefreshTransform);
			CameraRig->SyncOrbitFromCurrentView();
		}
	}
	else
	{
		CameraRig->SetOrbitTarget(FocusedTarget.GetPivotLocation(ActiveDeviceHost.Get()), bRefreshTransform);
	}

	if (FocusedTarget.Kind != EDIVEFocusKind::Anchor)
	{
		if (bUseDefaultOrbitDistance)
		{
			CameraRig->SetOrbitDistance(SessionDefaultOrbitDistance, bRefreshTransform);
			CameraRig->SyncOrbitOrientationFromCurrentView();
		}
		else
		{
			CameraRig->SyncOrbitFromCurrentView();
		}
	}

	const float BlendDuration = bBlendCamera ? Inspectable->FocusBlendDuration : 0.f;
	CameraRig->ApplyFocusPresentation(BlendDuration);

	Inspectable->UpdateAnchorSessionPresentation(FocusedTarget);

	if (bIsolationActive)
	{
		ApplyIsolationForTarget(FocusedTarget);
	}

	OnFocusChanged.Broadcast(FocusedTarget);
	return true;
}

bool UDIVESessionSubsystem::ApplyInitialSessionFocus(FName InitialFocusId)
{
	if (InitialFocusId.IsNone())
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	AActor* DeviceHost = ActiveDeviceHost.Get();
	if (!Inspectable || !DeviceHost)
	{
		return false;
	}

	FDIVEFocusTarget Target;
	if (!Inspectable->TryResolveStartFocusTarget(InitialFocusId, Target))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("DIVE: InitialFocusId '%s' not found on '%s'; falling back to device root."),
			*InitialFocusId.ToString(),
			*GetNameSafe(DeviceHost));
		return false;
	}

	FocusStack.Reset();
	FocusStack.Add(FDIVEFocusTarget::MakeDeviceRoot());
	const bool bUseDefaultOrbitDistance = Target.Kind == EDIVEFocusKind::Primitive;
	return ApplyFocusTarget(Target, true, true, bUseDefaultOrbitDistance);
}

bool UDIVESessionSubsystem::ApplyIsolationForTarget(const FDIVEFocusTarget& Target)
{
	if (!IsSessionActive() || Target.Kind == EDIVEFocusKind::DeviceRoot || !Target.IsValidFocus())
	{
		return false;
	}

	ClearPickHover();

	TArray<UPrimitiveComponent*> VisiblePrimitives;
	CollectIsolationVisiblePrimitives(Target, VisiblePrimitives);
	if (VisiblePrimitives.IsEmpty())
	{
		return false;
	}

	TArray<UPrimitiveComponent*> DevicePrimitives;
	DIVE::CollectDevicePrimitives(ActiveDeviceHost.Get(), DevicePrimitives);

	TSet<UPrimitiveComponent*> VisibleSet(VisiblePrimitives);
	TSet<UPrimitiveComponent*> ShouldHide;
	for (UPrimitiveComponent* Primitive : DevicePrimitives)
	{
		if (Primitive && !VisibleSet.Contains(Primitive))
		{
			ShouldHide.Add(Primitive);
		}
	}

	TSet<UPrimitiveComponent*> PreviouslyHidden;
	for (const TWeakObjectPtr<UPrimitiveComponent>& WeakPrimitive : IsolatedHiddenPrimitives)
	{
		if (UPrimitiveComponent* Primitive = WeakPrimitive.Get())
		{
			PreviouslyHidden.Add(Primitive);
			if (!ShouldHide.Contains(Primitive))
			{
				Primitive->SetHiddenInGame(false);
			}
		}
	}

	IsolatedHiddenPrimitives.Reset();
	for (UPrimitiveComponent* Primitive : ShouldHide)
	{
		if (!PreviouslyHidden.Contains(Primitive))
		{
			Primitive->SetHiddenInGame(true);
		}

		IsolatedHiddenPrimitives.Add(Primitive);
	}

	bIsolationActive = !IsolatedHiddenPrimitives.IsEmpty();
	if (bIsolationActive)
	{
		IsolationTarget = Target;
	}
	else
	{
		IsolationTarget = FDIVEFocusTarget::MakeDeviceRoot();
	}

	return bIsolationActive;
}

void UDIVESessionSubsystem::CollectIsolationVisiblePrimitives(
	const FDIVEFocusTarget& Target,
	TArray<UPrimitiveComponent*>& OutVisible) const
{
	AActor* DeviceHost = ActiveDeviceHost.Get();
	if (!DeviceHost)
	{
		return;
	}

	auto AddAncestorPrimitives = [DeviceHost, &OutVisible](USceneComponent* StartComponent)
	{
		for (USceneComponent* Current = StartComponent; Current; Current = Current->GetAttachParent())
		{
			if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Current))
			{
				OutVisible.AddUnique(Primitive);
			}

			if (Current->GetOwner() == DeviceHost && Current == DeviceHost->GetRootComponent())
			{
				break;
			}
		}
	};

	if (Target.Kind == EDIVEFocusKind::Primitive && Target.Primitive)
	{
		AddAncestorPrimitives(Target.Primitive);
	}
	else if (Target.Kind == EDIVEFocusKind::Anchor && Target.Anchor)
	{
		DIVE::CollectAttachedPrimitives(Target.Anchor.Get(), OutVisible);
		AddAncestorPrimitives(Target.Anchor.Get());
	}
}
