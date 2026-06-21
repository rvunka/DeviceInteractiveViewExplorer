// Copyright (c) 2026. All Rights Reserved.

#include "Input/DIVELegacyKbmInputComponent.h"

#include "Components/InputComponent.h"
#include "DIVESessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UDIVELegacyKbmInputComponent::UDIVELegacyKbmInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;

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

	BindInput();
	if (!bInputBound && GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UDIVELegacyKbmInputComponent::BindInput));
	}
}

void UDIVELegacyKbmInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		ClearSessionPresentation(PlayerController);
	}

	bOrbitKeyHeld = false;
	bApplyPresentationNextTick = false;
	bHasLastOrbitMousePosition = false;
	Super::EndPlay(EndPlayReason);
}

void UDIVELegacyKbmInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInputBound)
	{
		BindInput();
	}

	UpdateSessionPresentation();

	if (bOrbitKeyHeld)
	{
		ApplyOrbitFromMouseDelta();
	}
}

UDIVESessionSubsystem* UDIVELegacyKbmInputComponent::GetSessionSubsystem() const
{
	if (const UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UDIVESessionSubsystem>();
		}
	}

	return nullptr;
}

APlayerController* UDIVELegacyKbmInputComponent::GetLocalPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

void UDIVELegacyKbmInputComponent::UpdateSessionPresentation()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	const bool bSessionActive = Subsystem && Subsystem->IsSessionActive();

	if (bSessionActive && PlayerController)
	{
		if (!bSessionPresentationActive && !bApplyPresentationNextTick)
		{
			bApplyPresentationNextTick = true;
			return;
		}

		bApplyPresentationNextTick = false;

		if (!bSessionPresentationActive)
		{
			ApplySessionInputMode(PlayerController);

			if (bOverrideCameraSensitivity)
			{
				Subsystem->ConfigureActiveCameraInput(OrbitSensitivity, ZoomSensitivity);
			}
			else
			{
				Subsystem->ApplyCameraInputFromInspectable();
			}

			bSessionPresentationActive = true;
		}
		else
		{
			MaintainSessionInputFlags(PlayerController);
		}
	}
	else if (bSessionPresentationActive)
	{
		bApplyPresentationNextTick = false;
		ClearSessionPresentation(PlayerController);
	}
}

void UDIVELegacyKbmInputComponent::ApplySessionInputMode(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);
	MaintainSessionInputFlags(PlayerController);
}

void UDIVELegacyKbmInputComponent::MaintainSessionInputFlags(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	PlayerController->SetIgnoreLookInput(true);
	PlayerController->SetIgnoreMoveInput(bIgnoreMoveInputInSession);
	PlayerController->bShowMouseCursor = bShowMouseCursorInSession;
}

void UDIVELegacyKbmInputComponent::ClearSessionPresentation(APlayerController* PlayerController)
{
	if (!bSessionPresentationActive)
	{
		return;
	}

	if (PlayerController)
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetIgnoreMoveInput(false);
		PlayerController->bShowMouseCursor = false;
	}

	bOrbitKeyHeld = false;
	bHasLastOrbitMousePosition = false;
	bSessionPresentationActive = false;
}

void UDIVELegacyKbmInputComponent::ApplyOrbitFromMouseDelta()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	float MouseDeltaX = 0.f;
	float MouseDeltaY = 0.f;
	PlayerController->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
	if (!FMath::IsNearlyZero(MouseDeltaX) || !FMath::IsNearlyZero(MouseDeltaY))
	{
		bHasLastOrbitMousePosition = false;
		Subsystem->ApplyOrbitInput(FVector2D(MouseDeltaX, MouseDeltaY));
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const FVector2D CurrentPosition(MouseX, MouseY);
	if (!bHasLastOrbitMousePosition)
	{
		LastOrbitMousePosition = CurrentPosition;
		bHasLastOrbitMousePosition = true;
		return;
	}

	const FVector2D Delta = CurrentPosition - LastOrbitMousePosition;
	LastOrbitMousePosition = CurrentPosition;
	if (!Delta.IsNearlyZero())
	{
		Subsystem->ApplyOrbitInput(Delta);
	}
}

void UDIVELegacyKbmInputComponent::OrbitPressed()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

	bOrbitKeyHeld = true;
	bHasLastOrbitMousePosition = false;
}

void UDIVELegacyKbmInputComponent::OrbitReleased()
{
	bOrbitKeyHeld = false;
	bHasLastOrbitMousePosition = false;

	APlayerController* PlayerController = GetLocalPlayerController();
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (PlayerController && Subsystem && Subsystem->IsSessionActive())
	{
		ApplySessionInputMode(PlayerController);
	}
}

void UDIVELegacyKbmInputComponent::ZoomIn()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->ApplyZoomInput(1.f);
		}
	}
}

void UDIVELegacyKbmInputComponent::ZoomOut()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->ApplyZoomInput(-1.f);
		}
	}
}

void UDIVELegacyKbmInputComponent::SelectPressed()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!Subsystem || !Subsystem->IsSessionActive() || !PlayerController)
	{
		return;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (PlayerController->GetMousePosition(MouseX, MouseY))
	{
		Subsystem->SelectAtScreenPosition(FVector2D(MouseX, MouseY), PlayerController);
	}

	ApplySessionInputMode(PlayerController);
}

void UDIVELegacyKbmInputComponent::NavigateBackPressed()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

	Subsystem->NavigateBack();

	if (!Subsystem->IsSessionActive() && PlayerController)
	{
		ClearSessionPresentation(PlayerController);
	}
}

void UDIVELegacyKbmInputComponent::ExitSessionPressed()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

	Subsystem->EndSession();

	if (PlayerController)
	{
		ClearSessionPresentation(PlayerController);
	}
}

void UDIVELegacyKbmInputComponent::ToggleIsolatePressed()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->ToggleIsolateFocused();
		}
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

	UInputComponent* InputComponent = Pawn->InputComponent;
	bool bBoundAny = false;

	if (bBindOrbitInput && OrbitKey.IsValid())
	{
		InputComponent->BindKey(OrbitKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::OrbitPressed);
		InputComponent->BindKey(OrbitKey, IE_Released, this, &UDIVELegacyKbmInputComponent::OrbitReleased);
		bBoundAny = true;
	}

	if (bBindZoomInput)
	{
		if (ZoomInKey.IsValid())
		{
			InputComponent->BindKey(ZoomInKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ZoomIn);
			bBoundAny = true;
		}

		if (ZoomOutKey.IsValid())
		{
			InputComponent->BindKey(ZoomOutKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ZoomOut);
			bBoundAny = true;
		}
	}

	if (bBindSelectInput && SelectKey.IsValid())
	{
		InputComponent->BindKey(SelectKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::SelectPressed);
		bBoundAny = true;
	}

	if (bBindBackInput && BackKey.IsValid())
	{
		InputComponent->BindKey(BackKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::NavigateBackPressed);
		bBoundAny = true;
	}

	if (bBindExitInput && ExitKey.IsValid())
	{
		InputComponent->BindKey(ExitKey, IE_Pressed, this, &UDIVELegacyKbmInputComponent::ExitSessionPressed);
		bBoundAny = true;
	}

	bInputBound = bBoundAny;
}
