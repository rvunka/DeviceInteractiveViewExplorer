// Copyright (c) 2026. All Rights Reserved.

#include "Input/DIVELegacyKbmInputComponent.h"

#include "Components/InputComponent.h"
#include "Input/DIVEInputComponent.h"
#include "GameFramework/Pawn.h"
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
	BackKey = EKeys::BackSpace;
	ExitKey = EKeys::Escape;
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
		InputComponent->HandleSelectPressed();
	}
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

void UDIVELegacyKbmInputComponent::BindInput()
{
	if (bInputBound)
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn || !Pawn->IsLocallyControlled() || !Pawn->InputComponent)
	{
		return;
	}

	UInputComponent* PawnInputComponent = Pawn->InputComponent;
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
		bBoundAny = true;
	}

	if (bBindBackInput && BackKey.IsValid())
	{
		PawnInputComponent->BindKey(BackKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::NavigateBackPressed);
		bBoundAny = true;
	}

	if (bBindExitInput && ExitKey.IsValid())
	{
		PawnInputComponent->BindKey(ExitKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ExitSessionPressed);
		bBoundAny = true;
	}

	bInputBound = bBoundAny;
}
