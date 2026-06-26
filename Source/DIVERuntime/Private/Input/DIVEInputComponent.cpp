// Copyright (c) 2026. All Rights Reserved.

#include "Input/DIVEInputComponent.h"

#include "DIVESessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

UDIVEInputComponent::UDIVEInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UDIVEInputComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UDIVEInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->EndSession();
		}
	}

	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		ClearSessionPresentation(PlayerController);
	}

	bOrbitKeyHeld = false;
	bApplyPresentationNextTick = false;
	bHasLastOrbitMousePosition = false;
	Super::EndPlay(EndPlayReason);
}

void UDIVEInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return;
	}

	UpdateSessionPresentation();

	if (bOrbitKeyHeld)
	{
		ApplyOrbitFromMouseDelta();
	}
}

UDIVESessionSubsystem* UDIVEInputComponent::GetSessionSubsystem() const
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

APlayerController* UDIVEInputComponent::GetLocalPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

bool UDIVEInputComponent::IsLocallyControlledOwner() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

void UDIVEInputComponent::UpdateSessionPresentation()
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
			CapturePreSessionInputState(PlayerController);
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

void UDIVEInputComponent::CapturePreSessionInputState(APlayerController* PlayerController)
{
	if (!PlayerController || bHasPreservedInputState)
	{
		return;
	}

	bPreservedShowMouseCursor = PlayerController->bShowMouseCursor;
	bPreservedEnableClickEvents = PlayerController->bEnableClickEvents;
	bPreservedEnableMouseOverEvents = PlayerController->bEnableMouseOverEvents;
	bPreservedUsedGameAndUI = PlayerController->bShowMouseCursor || PlayerController->bEnableClickEvents;

	if (const UWorld* World = PlayerController->GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			PreservedMouseCaptureMode = ViewportClient->GetMouseCaptureMode();
			PreservedMouseLockMode = ViewportClient->GetMouseLockMode();
		}
	}

	bHasPreservedInputState = true;
}

void UDIVEInputComponent::RestorePreSessionInputState(APlayerController* PlayerController)
{
	if (!PlayerController || !bHasPreservedInputState)
	{
		return;
	}

	if (bSessionAppliedInputFlags)
	{
		PlayerController->SetIgnoreLookInput(false);
		if (bIgnoreMoveInputInSession)
		{
			PlayerController->SetIgnoreMoveInput(false);
		}

		bSessionAppliedInputFlags = false;
	}

	if (bPreservedUsedGameAndUI)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(PreservedMouseLockMode);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
	}
	else
	{
		PlayerController->SetInputMode(FInputModeGameOnly());
	}

	if (UGameViewportClient* ViewportClient = PlayerController->GetWorld()->GetGameViewport())
	{
		ViewportClient->SetMouseCaptureMode(PreservedMouseCaptureMode);
		ViewportClient->SetMouseLockMode(PreservedMouseLockMode);
	}

	PlayerController->bEnableClickEvents = bPreservedEnableClickEvents;
	PlayerController->bEnableMouseOverEvents = bPreservedEnableMouseOverEvents;
	PlayerController->bShowMouseCursor = bPreservedShowMouseCursor;
	bHasPreservedInputState = false;
}

void UDIVEInputComponent::ApplySessionInputMode(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	PlayerController->SetInputMode(InputMode);

	if (!bSessionAppliedInputFlags)
	{
		PlayerController->SetIgnoreLookInput(true);
		if (bIgnoreMoveInputInSession)
		{
			PlayerController->SetIgnoreMoveInput(true);
		}

		bSessionAppliedInputFlags = true;
	}

	MaintainSessionInputFlags(PlayerController);
}

void UDIVEInputComponent::MaintainSessionInputFlags(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = bShowMouseCursorInSession;
}

void UDIVEInputComponent::ClearSessionPresentation(APlayerController* PlayerController)
{
	if (!bSessionPresentationActive)
	{
		return;
	}

	if (PlayerController)
	{
		if (bHasPreservedInputState)
		{
			RestorePreSessionInputState(PlayerController);
		}
		else
		{
			if (bSessionAppliedInputFlags)
			{
				PlayerController->SetIgnoreLookInput(false);
				if (bIgnoreMoveInputInSession)
				{
					PlayerController->SetIgnoreMoveInput(false);
				}

				bSessionAppliedInputFlags = false;
			}

			PlayerController->SetInputMode(FInputModeGameOnly());
			PlayerController->bShowMouseCursor = false;
		}
	}

	bOrbitKeyHeld = false;
	bHasLastOrbitMousePosition = false;
	bSessionPresentationActive = false;
	bSessionAppliedInputFlags = false;
}

void UDIVEInputComponent::ApplyOrbitFromMouseDelta()
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

void UDIVEInputComponent::HandleOrbitPressed()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

	bOrbitKeyHeld = true;
	bHasLastOrbitMousePosition = false;
}

void UDIVEInputComponent::HandleOrbitReleased()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	bOrbitKeyHeld = false;
	bHasLastOrbitMousePosition = false;

	APlayerController* PlayerController = GetLocalPlayerController();
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (PlayerController && Subsystem && Subsystem->IsSessionActive())
	{
		ApplySessionInputMode(PlayerController);
	}
}

void UDIVEInputComponent::HandleOrbitDelta(FVector2D Delta)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive() && !Delta.IsNearlyZero())
		{
			Subsystem->ApplyOrbitInput(Delta);
		}
	}
}

void UDIVEInputComponent::HandleZoomIn()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->ApplyZoomInput(1.f);
		}
	}
}

void UDIVEInputComponent::HandleZoomOut()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->ApplyZoomInput(-1.f);
		}
	}
}

void UDIVEInputComponent::HandleSelectPressed()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

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

void UDIVEInputComponent::HandleNavigateBack()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

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

void UDIVEInputComponent::HandleExitSession()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

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

void UDIVEInputComponent::HandleToggleIsolate()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->ToggleIsolateFocused();
		}
	}
}
