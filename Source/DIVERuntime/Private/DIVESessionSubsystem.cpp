// Copyright (c) 2026. All Rights Reserved.

#include "DIVESessionSubsystem.h"

#include "DIVEAnchorComponent.h"
#include "DIVECameraRig.h"
#include "DIVEHierarchy.h"
#include "DIVEInspectableComponent.h"
#include "DIVEProxyDrive.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVEProxyDriveTypes.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Utils/DIVEPick.h"
#include "Utils/DIVEPlayerQuery.h"
#include "Utils/DIVEContextMenu.h"
#include "Containers/Set.h"

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
	ApplyWorldDim();

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
	ClearProxyDrive();
	ClearWorldDim();
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
	if (!ResolveFocusAtScreenPosition(ScreenPosition, PlayerController, SelectedTarget))
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

	if (NewMode != EDIVESessionInteractionMode::Physical)
	{
		ClearProxyDrive();
	}

	InteractionMode = NewMode;
}

bool UDIVESessionSubsystem::FocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack)
{
	if (!IsSessionActive() || !Target.IsValidFocus())
	{
		return false;
	}

	return ApplyFocusTarget(Target, bPushToStack);
}

bool UDIVESessionSubsystem::FocusAnchor(FName PartId)
{
	if (!IsSessionActive() || PartId.IsNone())
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	if (!Inspectable)
	{
		return false;
	}

	FDIVEPartNode Node;
	if (!Inspectable->FindAnchorNode(PartId, Node))
	{
		return false;
	}

	USceneComponent* AnchorComponent = Node.SceneComponent.Get();
	if (!AnchorComponent)
	{
		return false;
	}

	FDIVEFocusTarget Target = FDIVEFocusTarget::FromAnchor(AnchorComponent, Node.PartId);
	return FocusTarget(Target, true);
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

	const bool bHasValidPick = ResolveFocusAtScreenPosition(ScreenPosition, PlayerController, OutPickTarget);
	DIVEContextMenu::BuildBuiltInEntries(this, OutPickTarget, bHasValidPick, OutEntries);

	if (UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get())
	{
		Inspectable->AppendContextMenuEntries(OutPickTarget, OutEntries);
	}

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
		return ToggleIsolateFocused();
	}

	if (ActionId == DIVE::kContextBack)
	{
		return NavigateBack();
	}

	if (UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get())
	{
		return Inspectable->ExecuteContextMenuAction(ActionId, PickTarget);
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

bool UDIVESessionSubsystem::ToggleIsolateFocused()
{
	if (!IsSessionActive())
	{
		return false;
	}

	if (bIsolationActive)
	{
		ClearIsolation();
		return true;
	}

	return ApplyIsolation();
}

void UDIVESessionSubsystem::ClearIsolation()
{
	if (!bIsolationActive)
	{
		return;
	}

	for (const TWeakObjectPtr<UPrimitiveComponent>& WeakPrimitive : IsolatedHiddenPrimitives)
	{
		if (UPrimitiveComponent* Primitive = WeakPrimitive.Get())
		{
			Primitive->SetHiddenInGame(false);
		}
	}

	IsolatedHiddenPrimitives.Reset();
	bIsolationActive = false;
}

void UDIVESessionSubsystem::ClearProxyDrive()
{
	if (bProxyDriving)
	{
		if (UObject* ProxyObject = ActiveProxyDrive.GetObject())
		{
			IDIVEProxyDrive::Execute_EndProxyDrive(ProxyObject, false);
		}
	}

	bProxyDriving = false;
	ActiveProxyDrive.Reset();
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
	Context.TraceChannel = Inspectable->GetEffectivePickTraceChannel();

	if (!DIVEPick::PickAtScreenPosition(Context, ScreenPosition, PlayerController, HitResult, PickTarget))
	{
		return false;
	}

	UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	IDIVEProxyDrive* ProxyDrive = DIVEProxyDriveResolve::FindProxyDriveForHit(HitComponent);
	UObject* ProxyObject = Cast<UObject>(ProxyDrive);
	if (!ProxyObject || !IDIVEProxyDrive::Execute_CanProxyDrive(ProxyObject))
	{
		return false;
	}

	FDIVEProxyDriveContext DriveContext;
	DriveContext.ScreenPosition = ScreenPosition;
	DriveContext.FocusTarget = PickTarget;
	DriveContext.HitComponent = HitComponent;

	if (!IDIVEProxyDrive::Execute_BeginProxyDrive(ProxyObject, DriveContext))
	{
		return false;
	}

	ActiveProxyDrive = ProxyObject;
	bProxyDriving = true;
	return true;
}

void UDIVESessionSubsystem::UpdateProxyDrive(const FVector2D& ScreenDelta)
{
	if (!bProxyDriving)
	{
		return;
	}

	if (UObject* ProxyObject = ActiveProxyDrive.GetObject())
	{
		IDIVEProxyDrive::Execute_ApplyProxyDriveDelta(ProxyObject, ScreenDelta);
	}
	else
	{
		ClearProxyDrive();
	}
}

void UDIVESessionSubsystem::EndProxyDrive(bool bCommit)
{
	if (!bProxyDriving)
	{
		return;
	}

	if (UObject* ProxyObject = ActiveProxyDrive.GetObject())
	{
		IDIVEProxyDrive::Execute_EndProxyDrive(ProxyObject, bCommit);
	}

	bProxyDriving = false;
	ActiveProxyDrive.Reset();
}

bool UDIVESessionSubsystem::ResolveFocusAtScreenPosition(
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
	Context.TraceChannel = Inspectable->GetEffectivePickTraceChannel();

	return DIVEPick::ResolveFocusAtScreenPosition(Context, ScreenPosition, PlayerController, OutTarget);
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
	if (FocusedTarget.Kind == EDIVEFocusKind::DeviceRoot)
	{
		CameraRig->SetOrbitTarget(FocusedTarget.GetPivotLocation(ActiveDeviceHost.Get()), bRefreshTransform);
	}
	else if (FocusedTarget.Kind == EDIVEFocusKind::Primitive)
	{
		CameraRig->SetOrbitTarget(FocusedTarget.GetPivotLocation(ActiveDeviceHost.Get()), bRefreshTransform);
	}
	else if (FocusedTarget.Kind == EDIVEFocusKind::Anchor)
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

	if (FocusedTarget.Kind == EDIVEFocusKind::DeviceRoot || FocusedTarget.Kind == EDIVEFocusKind::Primitive)
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

	const float BlendDuration = bBlendCamera ? Inspectable->GetEffectiveFocusBlendDuration() : 0.f;
	CameraRig->ApplyFocusPresentation(BlendDuration);

	Inspectable->UpdateAnchorSessionPresentation(FocusedTarget);

	if (bIsolationActive)
	{
		ApplyIsolation();
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

bool UDIVESessionSubsystem::ApplyIsolation()
{
	if (!IsSessionActive() || FocusedTarget.Kind == EDIVEFocusKind::DeviceRoot)
	{
		return false;
	}

	TArray<UPrimitiveComponent*> VisiblePrimitives;
	CollectIsolationVisiblePrimitives(FocusedTarget, VisiblePrimitives);
	if (VisiblePrimitives.IsEmpty())
	{
		return false;
	}

	TArray<UPrimitiveComponent*> DevicePrimitives;
	DIVE::CollectDevicePrimitives(ActiveDeviceHost.Get(), DevicePrimitives);

	ClearIsolation();

	TSet<UPrimitiveComponent*> VisibleSet(VisiblePrimitives);
	for (UPrimitiveComponent* Primitive : DevicePrimitives)
	{
		if (!VisibleSet.Contains(Primitive))
		{
			Primitive->SetHiddenInGame(true);
			IsolatedHiddenPrimitives.Add(Primitive);
		}
	}

	bIsolationActive = !IsolatedHiddenPrimitives.IsEmpty();
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

void UDIVESessionSubsystem::ApplyWorldDim()
{
	ClearWorldDim();

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	AActor* DeviceHost = ActiveDeviceHost.Get();
	UWorld* World = GetWorld();
	if (!Inspectable || !DeviceHost || !World)
	{
		return;
	}

	if (Inspectable->GetEffectiveWorldDimPolicy() != EDIVEWorldDimPolicy::HideNonDeviceActors)
	{
		return;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || Actor->IsHidden())
		{
			continue;
		}

		if (Actor == DeviceHost || Actor == ActiveCameraRig.Get() || DIVE::IsDeviceActor(DeviceHost, Actor))
		{
			continue;
		}

		Actor->SetActorHiddenInGame(true);
		WorldDimHiddenActors.Add(Actor);
	}
}

void UDIVESessionSubsystem::ClearWorldDim()
{
	for (const TWeakObjectPtr<AActor>& WeakActor : WorldDimHiddenActors)
	{
		if (AActor* Actor = WeakActor.Get())
		{
			Actor->SetActorHiddenInGame(false);
		}
	}

	WorldDimHiddenActors.Reset();
}
