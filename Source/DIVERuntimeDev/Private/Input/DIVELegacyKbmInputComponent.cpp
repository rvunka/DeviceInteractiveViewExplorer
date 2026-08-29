// Copyright (c) 2026. All Rights Reserved.

#include "Input/DIVELegacyKbmInputComponent.h"

#include "Components/InputComponent.h"
#include "DIVELog.h"
#include "DIVESessionSubsystem.h"
#include "DIVETypes.h"
#include "DIVEInspectableComponent.h"
#include "DIVEPlayerComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Utils/SharedComponentResolve.h"
#include "Utils/DIVEGripLegacyDevQuery.h"

UDIVELegacyKbmInputComponent::UDIVELegacyKbmInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	OrbitKey = EKeys::MiddleMouseButton;
	ZoomInKey = EKeys::MouseScrollUp;
	ZoomOutKey = EKeys::MouseScrollDown;
	SelectKey = EKeys::LeftMouseButton;
	ManualRotateKey = EKeys::R;
	FocusUnderCursorKey = EKeys::G;
	InteractionModeCycleKey = EKeys::Tab;
	CameraUndoKey = EKeys::Z;
	ExitKey = EKeys::BackSpace;
	IsolateKey = EKeys::I;
	ContextMenuKey = EKeys::RightMouseButton;
}

void UDIVELegacyKbmInputComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveComponentReferences();
	if (!DivePlayer)
	{
		WarnMissingPlayerOnce();
	}

	BindSessionDelegates();
	RefreshSessionInputBindings();
}

void UDIVELegacyKbmInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSessionDelegates();
	UnbindInput();
	Super::EndPlay(EndPlayReason);
}

bool UDIVELegacyKbmInputComponent::IsDiveSessionActive() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const UDIVESessionSubsystem* DiveSubsystem = World->GetSubsystem<UDIVESessionSubsystem>();
	return DiveSubsystem && DiveSubsystem->IsSessionActive();
}

void UDIVELegacyKbmInputComponent::BindSessionDelegates()
{
	if (UWorld* World = GetWorld())
	{
		if (UDIVESessionSubsystem* DiveSubsystem = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			DiveSubsystem->OnSessionStarted.AddUniqueDynamic(this, &UDIVELegacyKbmInputComponent::HandleDiveSessionStarted);
			DiveSubsystem->OnSessionEnded.AddUniqueDynamic(this, &UDIVELegacyKbmInputComponent::HandleDiveSessionEnded);
		}
	}
}

void UDIVELegacyKbmInputComponent::UnbindSessionDelegates()
{
	if (UWorld* World = GetWorld())
	{
		if (UDIVESessionSubsystem* DiveSubsystem = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			DiveSubsystem->OnSessionStarted.RemoveDynamic(this, &UDIVELegacyKbmInputComponent::HandleDiveSessionStarted);
			DiveSubsystem->OnSessionEnded.RemoveDynamic(this, &UDIVELegacyKbmInputComponent::HandleDiveSessionEnded);
		}
	}
}

void UDIVELegacyKbmInputComponent::RefreshSessionInputBindings()
{
	if (IsDiveSessionActive())
	{
		BindInput();
		if (!bInputBound && GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &UDIVELegacyKbmInputComponent::BindInput));
		}
	}
	else
	{
		UnbindInput();
	}
}

void UDIVELegacyKbmInputComponent::HandleDiveSessionStarted(
	AActor* /*DeviceHost*/,
	UDIVEInspectableComponent* /*Inspectable*/)
{
	RefreshSessionInputBindings();
}

void UDIVELegacyKbmInputComponent::HandleDiveSessionEnded(
	EDIVESessionEndReason /*Reason*/,
	AActor* /*DeviceHost*/)
{
	UnbindInput();
}

void UDIVELegacyKbmInputComponent::ResolveComponentReferences()
{
	DivePlayer = SharedComponentResolve::FindComponentByNameOrClass<UDIVEPlayerComponent>(
		GetOwner(),
		PlayerComponentName);
}

void UDIVELegacyKbmInputComponent::WarnMissingPlayerOnce()
{
	if (bLoggedMissingPlayer)
	{
		return;
	}

	bLoggedMissingPlayer = true;
	UE_LOG(
		LogDIVE,
		Warning,
		TEXT("DIVE Legacy KBM Input on '%s': no DIVE Player component found. Add UDIVEPlayerComponent to the pawn."),
		*GetNameSafe(GetOwner()));
}

void UDIVELegacyKbmInputComponent::EnsurePlayerReady()
{
	if (!DivePlayer)
	{
		ResolveComponentReferences();
	}

	if (!bInputBound)
	{
		BindInput();
	}
}

void UDIVELegacyKbmInputComponent::OrbitPressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleOrbitPressed();
	}
}

void UDIVELegacyKbmInputComponent::OrbitReleased()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleOrbitReleased();
	}
}

bool UDIVELegacyKbmInputComponent::TryRouteZoomWheel(const float WheelDelta)
{
	if (DIVEGripLegacyDevQuery::TryForwardMouseWheelToGrip(GetOwner(), WheelDelta))
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const UDIVESessionSubsystem* DiveSubsystem = World->GetSubsystem<UDIVESessionSubsystem>();
	// Suppress orbit zoom while any session gesture is live (Interact hold or Physical GRIP).
	return DiveSubsystem && DiveSubsystem->IsSessionGestureActive();
}

void UDIVELegacyKbmInputComponent::ZoomIn()
{
	EnsurePlayerReady();

	if (TryRouteZoomWheel(1.0f))
	{
		return;
	}

	if (DivePlayer)
	{
		DivePlayer->HandleZoomIn();
	}
}

void UDIVELegacyKbmInputComponent::ZoomOut()
{
	EnsurePlayerReady();

	if (TryRouteZoomWheel(-1.0f))
	{
		return;
	}

	if (DivePlayer)
	{
		DivePlayer->HandleZoomOut();
	}
}

void UDIVELegacyKbmInputComponent::SelectPressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandlePrimaryActionPressed();
	}
}

void UDIVELegacyKbmInputComponent::SelectReleased()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandlePrimaryActionReleased();
	}
}

void UDIVELegacyKbmInputComponent::ManualRotatePressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleManualRotatePressed();
	}
}

void UDIVELegacyKbmInputComponent::ManualRotateReleased()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleManualRotateReleased();
	}
}

void UDIVELegacyKbmInputComponent::FocusUnderCursorPressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleFocusUnderCursor();
	}
}

void UDIVELegacyKbmInputComponent::CycleInteractionModePressed()
{
	EnsurePlayerReady();

	if (!DivePlayer)
	{
		return;
	}

	DivePlayer->HandleCycleInteractionMode();
}

void UDIVELegacyKbmInputComponent::NavigateBackPressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleNavigateBack();
	}
}

void UDIVELegacyKbmInputComponent::ExitSessionPressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleExitSession();
	}
}

void UDIVELegacyKbmInputComponent::ToggleIsolatePressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleToggleIsolate();
	}
}

void UDIVELegacyKbmInputComponent::ContextMenuPressed()
{
	EnsurePlayerReady();

	if (DivePlayer)
	{
		DivePlayer->HandleContextMenuRequested();
	}
}

void UDIVELegacyKbmInputComponent::ClearLegacyKeyBindings()
{
	if (!LegacyInputComponent)
	{
		return;
	}

	// BindKey writes KeyBindings. ClearActionBindings() does not touch that array.
	LegacyInputComponent->KeyBindings.Reset();
}

void UDIVELegacyKbmInputComponent::UnbindInput()
{
	if (LegacyInputComponent)
	{
		if (APawn* Pawn = Cast<APawn>(GetOwner()))
		{
			if (APlayerController* PlayerController = Pawn->GetController<APlayerController>())
			{
				// Safe if not on the stack (returns false). Prefer Pop whenever we own a component.
				PlayerController->PopInputComponent(LegacyInputComponent);
			}
		}
	}

	ClearLegacyKeyBindings();
	bInputBound = false;
}

void UDIVELegacyKbmInputComponent::BindInput()
{
	if (bInputBound || !IsDiveSessionActive())
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled())
	{
		return;
	}

	if (!LegacyInputComponent)
	{
		LegacyInputComponent = NewObject<UInputComponent>(this, TEXT("DIVE_LegacyInput"));
		LegacyInputComponent->RegisterComponent();
	}

	// Always start from a clean KeyBindings list before re-binding.
	ClearLegacyKeyBindings();

	UInputComponent* PawnInputComponent = LegacyInputComponent;
	bool bBoundAny = false;

	if (bBindOrbitInput && OrbitKey.IsValid())
	{
		PawnInputComponent->BindKey(OrbitKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::OrbitPressed);
		PawnInputComponent->BindKey(OrbitKey, IE_Released, this, &UDIVELegacyKbmInputComponent::OrbitReleased);
		bBoundAny = true;
	}

	if (bBindZoomInput)
	{
		if (ZoomInKey.IsValid())
		{
			PawnInputComponent->BindKey(ZoomInKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ZoomIn);
			bBoundAny = true;
		}

		if (ZoomOutKey.IsValid())
		{
			PawnInputComponent->BindKey(ZoomOutKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ZoomOut);
			bBoundAny = true;
		}
	}

	if (bBindSelectInput && SelectKey.IsValid())
	{
		PawnInputComponent->BindKey(SelectKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::SelectPressed);
		PawnInputComponent->BindKey(SelectKey, IE_Released, this, &UDIVELegacyKbmInputComponent::SelectReleased);
		bBoundAny = true;
	}

	if (bBindManualRotateInput && ManualRotateKey.IsValid())
	{
		PawnInputComponent->BindKey(ManualRotateKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ManualRotatePressed);
		PawnInputComponent->BindKey(ManualRotateKey, IE_Released, this, &UDIVELegacyKbmInputComponent::ManualRotateReleased);
		bBoundAny = true;
	}

	if (bBindFocusUnderCursorInput && FocusUnderCursorKey.IsValid())
	{
		PawnInputComponent->BindKey(FocusUnderCursorKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::FocusUnderCursorPressed);
		bBoundAny = true;
	}

	if (bBindInteractionModeCycleInput && InteractionModeCycleKey.IsValid())
	{
		PawnInputComponent->BindKey(InteractionModeCycleKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::CycleInteractionModePressed);
		bBoundAny = true;
	}

	if (bBindCameraUndoInput && CameraUndoKey.IsValid())
	{
		const FInputChord UndoChord(CameraUndoKey, false, true, false, false);
		PawnInputComponent->BindKey(UndoChord, IE_Pressed, this, &UDIVELegacyKbmInputComponent::NavigateBackPressed);
		bBoundAny = true;
	}

	if (bBindExitInput && ExitKey.IsValid())
	{
		PawnInputComponent->BindKey(ExitKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ExitSessionPressed);
		bBoundAny = true;
	}

	if (bBindIsolateInput && IsolateKey.IsValid())
	{
		PawnInputComponent->BindKey(IsolateKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ToggleIsolatePressed);
		bBoundAny = true;
	}

	if (bBindContextMenuInput && ContextMenuKey.IsValid())
	{
		PawnInputComponent->BindKey(ContextMenuKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ContextMenuPressed);
		bBoundAny = true;
	}

	bInputBound = bBoundAny;
	if (bInputBound)
	{
		if (APlayerController* PlayerController = Pawn->GetController<APlayerController>())
		{
			PlayerController->PushInputComponent(LegacyInputComponent);
		}
		else
		{
			ClearLegacyKeyBindings();
			bInputBound = false;
		}
	}
}
