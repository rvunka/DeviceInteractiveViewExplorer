// Copyright (c) 2026. All Rights Reserved.

#include "Input/DIVELegacyKbmInputComponent.h"

#include "Components/InputComponent.h"
#include "DIVETypes.h"
#include "Input/DIVEInputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "TimerManager.h"
#include "Utils/DIVEComponentResolve.h"

UDIVELegacyKbmInputComponent::UDIVELegacyKbmInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	OrbitKey = EKeys::MiddleMouseButton;
	ZoomInKey = EKeys::MouseScrollUp;
	ZoomOutKey = EKeys::MouseScrollDown;
	SelectKey = EKeys::LeftMouseButton;
	ManualRotateKey = EKeys::R;
	FocusUnderCursorKey = EKeys::G;
	InteractionModeCycleKey = EKeys::LeftAlt;
	CameraUndoKey = EKeys::Z;
	ExitKey = EKeys::BackSpace;
	IsolateKey = EKeys::I;
	ContextMenuKey = EKeys::RightMouseButton;
}

void UDIVELegacyKbmInputComponent::BeginPlay()
{
	Super::BeginPlay();

	ResolveComponentReferences();
	if (!InputComponent)
	{
		WarnMissingInputOnce();
	}

	BindInput();
	if (!bInputBound && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UDIVELegacyKbmInputComponent::BindInput));
	}
}

void UDIVELegacyKbmInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindInput();
	Super::EndPlay(EndPlayReason);
}

void UDIVELegacyKbmInputComponent::ResolveComponentReferences()
{
	InputComponent = DIVEComponentResolve::FindComponentByNameOrClass<UDIVEInputComponent>(
		GetOwner(),
		InputComponentName);
}

void UDIVELegacyKbmInputComponent::WarnMissingInputOnce()
{
	if (bLoggedMissingInput)
	{
		return;
	}

	bLoggedMissingInput = true;
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("DIVE Legacy KBM Input on '%s': no DIVE Input component found. Link Input Component or add UDIVEInputComponent to the pawn."),
		*GetNameSafe(GetOwner()));
}

void UDIVELegacyKbmInputComponent::EnsureInputReady()
{
	if (!InputComponent)
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
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleOrbitPressed();
	}
}

void UDIVELegacyKbmInputComponent::OrbitReleased()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleOrbitReleased();
	}
}

void UDIVELegacyKbmInputComponent::ZoomIn()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleZoomIn();
	}
}

void UDIVELegacyKbmInputComponent::ZoomOut()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleZoomOut();
	}
}

void UDIVELegacyKbmInputComponent::SelectPressed()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandlePrimaryActionPressed();
	}
}

void UDIVELegacyKbmInputComponent::SelectReleased()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandlePrimaryActionReleased();
	}
}

void UDIVELegacyKbmInputComponent::ManualRotatePressed()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleManualRotatePressed();
	}
}

void UDIVELegacyKbmInputComponent::ManualRotateReleased()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleManualRotateReleased();
	}
}

void UDIVELegacyKbmInputComponent::FocusUnderCursorPressed()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleFocusUnderCursor();
	}
}

void UDIVELegacyKbmInputComponent::CycleInteractionModePressed()
{
	EnsureInputReady();

	if (!InputComponent)
	{
		return;
	}

	InputComponent->HandleCycleInteractionMode();
}

void UDIVELegacyKbmInputComponent::NavigateBackPressed()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleNavigateBack();
	}
}

void UDIVELegacyKbmInputComponent::ExitSessionPressed()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleExitSession();
	}
}

void UDIVELegacyKbmInputComponent::ToggleIsolatePressed()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleToggleIsolate();
	}
}

void UDIVELegacyKbmInputComponent::ContextMenuPressed()
{
	EnsureInputReady();

	if (InputComponent)
	{
		InputComponent->HandleContextMenuRequested();
	}
}

void UDIVELegacyKbmInputComponent::UnbindInput()
{
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (LegacyInputComponent && bInputBound)
		{
			if (APlayerController* PlayerController = Pawn->GetController<APlayerController>())
			{
				PlayerController->PopInputComponent(LegacyInputComponent);
			}
		}
	}

	if (LegacyInputComponent)
	{
		LegacyInputComponent->ClearActionBindings();
	}

	bInputBound = false;
}

void UDIVELegacyKbmInputComponent::BindInput()
{
	if (bInputBound)
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
			bInputBound = false;
		}
	}
}
