// Copyright (c) 2026. All Rights Reserved.

#include "DIVESessionSubsystem.h"

#include "DIVEAnchorComponent.h"
#include "DIVECameraRig.h"
#include "DIVEConvention.h"
#include "DIVEHierarchy.h"
#include "Components/StaticMeshComponent.h"
#include "DIVEInspectableComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Set.h"

namespace
{
APlayerController* FindLocalPlayerController(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			if (PlayerController->IsLocalController())
			{
				return PlayerController;
			}
		}
	}

	return nullptr;
}

bool IsComponentPartOfDeviceHost(const USceneComponent* Component, const AActor* DeviceHost)
{
	if (!Component || !DeviceHost)
	{
		return false;
	}

	if (const AActor* Owner = Component->GetOwner())
	{
		if (Owner == DeviceHost || Owner->IsAttachedTo(DeviceHost))
		{
			return true;
		}
	}

	for (const USceneComponent* Current = Component; Current; Current = Current->GetAttachParent())
	{
		if (Current->GetOwner() == DeviceHost)
		{
			return true;
		}
	}

	return false;
}
}

void UDIVESessionSubsystem::Deinitialize()
{
	EndSession();
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

	if (SessionState == EDIVESessionState::Active)
	{
		EndSession();
	}

	Inspectable->BuildSemanticRegistry();

	UWorld* World = GetWorld();
	APlayerController* PlayerController = FindLocalPlayerController(World);
	if (!PlayerController)
	{
		return false;
	}

	ActiveDeviceHost = DeviceHost;
	ActiveInspectable = Inspectable;
	HoveredTarget = FDIVEFocusTarget::MakeDeviceRoot();
	FocusStack.Reset();
	FocusStack.Add(FDIVEFocusTarget::MakeDeviceRoot());

	if (AActor* CurrentViewTarget = PlayerController->GetViewTarget())
	{
		PreviousViewTarget = CurrentViewTarget;
	}

	const FVector FocusLocation = FocusStack.Last().GetPivotLocation(DeviceHost);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	ADIVECameraRig* CameraRig = World->SpawnActor<ADIVECameraRig>(ADIVECameraRig::StaticClass(), FocusLocation, FRotator::ZeroRotator, SpawnParams);
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
	CameraRig->SetOrbitDistance(DefaultDistance);
	CameraRig->SetOrbitTarget(FocusLocation);
	ApplyCameraInputFromInspectable();
	ActiveCameraRig = CameraRig;
	SessionState = EDIVESessionState::Active;

	PlayerController->SetViewTargetWithBlend(CameraRig, 0.f);
	Inspectable->NotifySessionLifecycle(true);

	if (!Params.InitialAnchorId.IsNone())
	{
		if (!FocusAnchor(Params.InitialAnchorId))
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("DIVE: InitialAnchorId '%s' not found on '%s'; falling back to device root."),
				*Params.InitialAnchorId.ToString(),
				*GetNameSafe(DeviceHost));
			ApplyFocusTarget(FocusStack.Last(), false);
		}
	}
	else
	{
		ApplyFocusTarget(FocusStack.Last(), false);
	}

	return true;
}

void UDIVESessionSubsystem::EndSession()
{
	if (SessionState == EDIVESessionState::Inactive)
	{
		return;
	}

	ClearIsolation();
	ClearHighlightPrimitives(HighlightedHoverPrimitives);
	ClearHighlightPrimitives(HighlightedFocusPrimitives);
	HoveredTarget = FDIVEFocusTarget::MakeDeviceRoot();
	FocusStack.Reset();

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = FindLocalPlayerController(World))
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
}

void UDIVESessionSubsystem::ApplyOrbitInput(const FVector2D& Delta)
{
	if (ADIVECameraRig* CameraRig = ActiveCameraRig.Get())
	{
		CameraRig->ApplyOrbitDelta(Delta);
	}
}

void UDIVESessionSubsystem::ApplyZoomInput(float Delta)
{
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

bool UDIVESessionSubsystem::SelectAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController)
{
	FDIVEFocusTarget SelectedTarget;
	if (!ResolveFocusAtScreenPosition(ScreenPosition, PlayerController, SelectedTarget))
	{
		return false;
	}

	return FocusTarget(SelectedTarget, true);
}

bool UDIVESessionSubsystem::UpdateHoverAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController)
{
	if (!IsSessionActive())
	{
		return false;
	}

	FDIVEFocusTarget NewHoveredTarget;
	if (ResolveFocusAtScreenPosition(ScreenPosition, PlayerController, NewHoveredTarget))
	{
		if (!NewHoveredTarget.Equals(HoveredTarget))
		{
			HoveredTarget = NewHoveredTarget;
			RefreshHighlights();
		}

		return true;
	}

	if (HoveredTarget.Kind != EDIVEFocusKind::DeviceRoot)
	{
		HoveredTarget = FDIVEFocusTarget::MakeDeviceRoot();
		RefreshHighlights();
	}

	return false;
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

	if (FocusStack.Num() > 1)
	{
		FocusStack.Pop();
		return ApplyFocusTarget(FocusStack.Last(), false);
	}

	EndSession();
	return true;
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

bool UDIVESessionSubsystem::RequestFocusedOperation(FName OperationId, FDIVEOperationResult& OutResult)
{
	if (!IsSessionActive())
	{
		OutResult.bSuccess = false;
		OutResult.Message = NSLOCTEXT("DIVE", "NoActiveSession", "No active DIVE session.");
		return false;
	}

	UDIVEInspectableComponent* Inspectable = ActiveInspectable.Get();
	if (!Inspectable)
	{
		OutResult.bSuccess = false;
		OutResult.Message = NSLOCTEXT("DIVE", "MissingInspectable", "Inspectable component missing.");
		return false;
	}

	FDIVEOperationRequest Request;
	Request.OperationId = OperationId;
	Request.FocusTarget = FocusedTarget;
	Request.SemanticPartId = FocusedTarget.SemanticPartId;
	return Inspectable->RequestOperation(Request, OutResult);
}

void UDIVESessionSubsystem::Tick(float /*DeltaTime*/)
{
	if (!IsSessionActive())
	{
		return;
	}

	APlayerController* PlayerController = FindLocalPlayerController(GetWorld());
	if (!PlayerController)
	{
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	UpdateHoverAtScreenPosition(FVector2D(MouseX, MouseY), PlayerController);
}

TStatId UDIVESessionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDIVESessionSubsystem, STATGROUP_Tickables);
}

bool UDIVESessionSubsystem::IsTickable() const
{
	return IsSessionActive();
}

UWorld* UDIVESessionSubsystem::GetTickableGameObjectWorld() const
{
	return GetWorld();
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

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!UGameplayStatics::DeprojectScreenToWorld(PlayerController, ScreenPosition, WorldOrigin, WorldDirection))
	{
		return false;
	}

	FHitResult HitResult;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(DIVEPick), false);
	QueryParams.AddIgnoredActor(ActiveCameraRig.Get());

	if (!GetWorld()->LineTraceSingleByChannel(HitResult, WorldOrigin, WorldOrigin + WorldDirection * 100000.f, ECC_Visibility, QueryParams))
	{
		return false;
	}

	UPrimitiveComponent* HitPrimitive = HitResult.GetComponent();
	if (!HitPrimitive || !IsComponentPartOfDeviceHost(HitPrimitive, DeviceHost))
	{
		return false;
	}

	const bool bIsAnchorMarker = HitPrimitive->ComponentHasTag(DIVE::kAnchorMarkerTag);
	if (!bIsAnchorMarker && !Inspectable->IsPrimitivePickable(HitPrimitive))
	{
		return false;
	}

	if (bIsAnchorMarker)
	{
		if (UDIVEAnchorComponent* Anchor = DIVE::FindAncestorComponent<UDIVEAnchorComponent>(HitPrimitive))
		{
			OutTarget = FDIVEFocusTarget::FromAnchor(Anchor, Anchor->GetResolvedPartId());
			return true;
		}

		return false;
	}

	const FName SemanticPartId = Inspectable->ResolveSemanticPartId(HitPrimitive);
	OutTarget = FDIVEFocusTarget::FromPrimitive(HitPrimitive, SemanticPartId);
	return true;
}

bool UDIVESessionSubsystem::ApplyFocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack)
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

	if (FocusedTarget.Kind == EDIVEFocusKind::DeviceRoot)
	{
		CameraRig->SetOrbitTarget(FocusedTarget.GetPivotLocation(ActiveDeviceHost.Get()));
		CameraRig->SetOrbitDistance(SessionDefaultOrbitDistance);
	}
	else if (FocusedTarget.Kind == EDIVEFocusKind::Primitive)
	{
		CameraRig->SetOrbitTarget(FocusedTarget.GetPivotLocation(ActiveDeviceHost.Get()));
	}
	else if (FocusedTarget.Kind == EDIVEFocusKind::Anchor)
	{
		if (UDIVEAnchorComponent* Anchor = Cast<UDIVEAnchorComponent>(FocusedTarget.Anchor.Get()))
		{
			CameraRig->SetAnchorViewpoint(Anchor->GetComponentLocation(), Anchor->GetViewRotation());
		}
		else
		{
			CameraRig->SetOrbitTarget(FocusedTarget.GetPivotLocation(ActiveDeviceHost.Get()));
		}
	}
	RefreshHighlights();

	Inspectable->UpdateAnchorSessionPresentation(FocusedTarget);

	if (bIsolationActive)
	{
		ApplyIsolation();
	}

	return true;
}

void UDIVESessionSubsystem::RefreshHighlights()
{
	ClearHighlightPrimitives(HighlightedHoverPrimitives);
	ClearHighlightPrimitives(HighlightedFocusPrimitives);

	if (HoveredTarget.Kind == EDIVEFocusKind::Primitive && HoveredTarget.Primitive
		&& !HoveredTarget.Equals(FocusedTarget))
	{
		ApplyHighlightForPrimitive(HoveredTarget.Primitive, DIVE::kHoverStencilValue, HighlightedHoverPrimitives);
	}
	else if (HoveredTarget.Kind == EDIVEFocusKind::Anchor && HoveredTarget.Anchor
		&& !HoveredTarget.Equals(FocusedTarget))
	{
		if (UDIVEAnchorComponent* Anchor = Cast<UDIVEAnchorComponent>(HoveredTarget.Anchor.Get()))
		{
			if (UStaticMeshComponent* MarkerMesh = Anchor->GetSessionMarkerMesh())
			{
				ApplyHighlightForPrimitive(MarkerMesh, DIVE::kHoverStencilValue, HighlightedHoverPrimitives);
			}
		}
	}

	if (FocusedTarget.Kind == EDIVEFocusKind::Primitive && FocusedTarget.Primitive)
	{
		ApplyHighlightForPrimitive(FocusedTarget.Primitive, DIVE::kFocusStencilValue, HighlightedFocusPrimitives);
	}
}

void UDIVESessionSubsystem::ClearHighlightPrimitives(TArray<TWeakObjectPtr<UPrimitiveComponent>>& Primitives)
{
	for (const TWeakObjectPtr<UPrimitiveComponent>& WeakPrimitive : Primitives)
	{
		if (UPrimitiveComponent* Primitive = WeakPrimitive.Get())
		{
			Primitive->SetRenderCustomDepth(false);
		}
	}

	Primitives.Reset();
}

void UDIVESessionSubsystem::ApplyHighlightForPrimitive(
	UPrimitiveComponent* Primitive,
	int32 StencilValue,
	TArray<TWeakObjectPtr<UPrimitiveComponent>>& OutTrackedPrimitives)
{
	if (!Primitive)
	{
		return;
	}

	Primitive->SetRenderCustomDepth(true);
	Primitive->SetCustomDepthStencilValue(StencilValue);
	OutTrackedPrimitives.Add(Primitive);
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
	CollectDevicePrimitives(DevicePrimitives);

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

void UDIVESessionSubsystem::CollectDevicePrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	AActor* DeviceHost = ActiveDeviceHost.Get();
	if (!DeviceHost)
	{
		return;
	}

	TSet<AActor*> ProcessedActors;
	TArray<AActor*> ActorStack;
	ActorStack.Add(DeviceHost);

	while (ActorStack.Num() > 0)
	{
		AActor* Actor = ActorStack.Pop(EAllowShrinking::No);
		if (!Actor || ProcessedActors.Contains(Actor))
		{
			continue;
		}

		ProcessedActors.Add(Actor);
		Actor->GetComponents<UPrimitiveComponent>(OutPrimitives);

		TArray<AActor*> AttachedActors;
		Actor->GetAttachedActors(AttachedActors);
		ActorStack.Append(AttachedActors);
	}
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
