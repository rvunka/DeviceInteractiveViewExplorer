// Copyright (c) 2026. All Rights Reserved.

#include "Input/DIVEInputComponent.h"

#include "DIVESessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/DIVEContextMenuUIComponent.h"
#include "Utils/DIVEComponentResolve.h"

UDIVEInputComponent::UDIVEInputComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UDIVEInputComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
	ResolveComponentReferences();
	BindSessionDelegates();

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			HandleSessionStarted(Subsystem->GetActiveDeviceHost(), Subsystem->GetActiveInspectable());
		}
	}
}

void UDIVEInputComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSessionDelegates();

	if (IsLocallyControlledOwner())
	{
		if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
		{
			if (Subsystem->IsSessionActive())
			{
				Subsystem->EndSession(EDIVESessionEndReason::Forced);
			}
		}

		if (APlayerController* PlayerController = GetLocalPlayerController())
		{
			ClearSessionPresentation(PlayerController);
		}
	}

	SetComponentTickEnabled(false);
	bOrbitKeyHeld = false;
	bPrimaryActionHeld = false;
	bPrimaryActionDragActive = false;
	bHasLastOrbitMousePosition = false;
	Super::EndPlay(EndPlayReason);
}

void UDIVEInputComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (bOrbitKeyHeld)
	{
		ApplyOrbitFromMouseDelta();
	}

	if (bPrimaryActionHeld && !ShouldSuppressSessionInput())
	{
		ApplyPrimaryActionDragFromMouse();
	}
}

void UDIVEInputComponent::BindSessionDelegates()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->OnSessionStarted.AddDynamic(this, &UDIVEInputComponent::HandleSessionStarted);
		Subsystem->OnSessionEnded.AddDynamic(this, &UDIVEInputComponent::HandleSessionEnded);
	}
}

void UDIVEInputComponent::UnbindSessionDelegates()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->OnSessionStarted.RemoveDynamic(this, &UDIVEInputComponent::HandleSessionStarted);
		Subsystem->OnSessionEnded.RemoveDynamic(this, &UDIVEInputComponent::HandleSessionEnded);
	}
}

void UDIVEInputComponent::HandleSessionStarted(AActor* /*DeviceHost*/, UDIVEInspectableComponent* /*Inspectable*/)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	SetComponentTickEnabled(true);
	BeginSessionPresentation();
}

void UDIVEInputComponent::HandleSessionEnded(EDIVESessionEndReason /*Reason*/, AActor* /*DeviceHost*/)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		ClearSessionPresentation(PlayerController);
	}

	bOrbitKeyHeld = false;
	bPrimaryActionHeld = false;
	bPrimaryActionDragActive = false;
	bHasLastOrbitMousePosition = false;
	SetComponentTickEnabled(false);
}

void UDIVEInputComponent::BeginSessionPresentation()
{
	if (bSessionPresentationActive)
	{
		return;
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!PlayerController || !Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

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

void UDIVEInputComponent::ResolveComponentReferences()
{
	ContextMenuUIComponent = DIVEComponentResolve::FindComponentByNameOrClass<UDIVEContextMenuUIComponent>(
		GetOwner(),
		ContextMenuUIComponentName);
}

void UDIVEInputComponent::WarnMissingContextMenuUIOnce()
{
	if (bLoggedMissingContextMenuUI || ContextMenuUIComponent)
	{
		return;
	}

	bLoggedMissingContextMenuUI = true;
	UE_LOG(
		LogTemp,
		Warning,
		TEXT("DIVE Input on '%s': no UDIVEContextMenuUIComponent found on the pawn — add it to show the context menu widget."),
		*GetNameSafe(GetOwner()));
}

void UDIVEInputComponent::ApplyPrimaryActionDragFromMouse()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem || !Subsystem->IsProxyDriving())
	{
		return;
	}

	FVector2D CurrentPosition;
	if (!TryGetCursorScreenPosition(CurrentPosition))
	{
		return;
	}

	if (!bPrimaryActionDragActive)
	{
		if (FVector2D::DistSquared(CurrentPosition, PrimaryActionLastPosition) >= 4.f)
		{
			bPrimaryActionDragActive = true;
		}
		else
		{
			return;
		}
	}

	const FVector2D Delta = CurrentPosition - PrimaryActionLastPosition;
	PrimaryActionLastPosition = CurrentPosition;
	if (!Delta.IsNearlyZero())
	{
		Subsystem->UpdateProxyDrive(Delta);
	}
}

bool UDIVEInputComponent::TryGetCursorScreenPosition(FVector2D& OutScreenPosition) const
{
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		return false;
	}

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return false;
	}

	OutScreenPosition = FVector2D(MouseX, MouseY);
	return true;
}

void UDIVEInputComponent::RoutePrimaryActionPressed(const FVector2D& ScreenPosition)
{
	if (ShouldSuppressSessionInput())
	{
		return;
	}

	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!Subsystem || !PlayerController)
	{
		return;
	}

	PrimaryActionLastPosition = ScreenPosition;
	bPrimaryActionHeld = true;
	bPrimaryActionDragActive = false;

	if (Subsystem->GetInteractionMode() != EDIVESessionInteractionMode::Physical)
	{
		bPrimaryActionHeld = false;
		return;
	}

	if (Subsystem->TryBeginProxyDriveAtScreenPosition(ScreenPosition, PlayerController))
	{
		return;
	}

	bPrimaryActionHeld = false;
}

void UDIVEInputComponent::RoutePrimaryActionReleased()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsProxyDriving())
		{
			Subsystem->EndProxyDrive(true);
		}
	}

	bPrimaryActionHeld = false;
	bPrimaryActionDragActive = false;
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

	if (UGameViewportClient* ViewportClient = PlayerController->GetWorld() ? PlayerController->GetWorld()->GetGameViewport() : nullptr)
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
	bPrimaryActionHeld = false;
	bPrimaryActionDragActive = false;
	bHasLastOrbitMousePosition = false;
	bSessionPresentationActive = false;
	bSessionAppliedInputFlags = false;
}

void UDIVEInputComponent::ApplyOrbitFromMouseDelta()
{
	if (ShouldSuppressSessionInput())
	{
		return;
	}

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
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
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
	if (!IsLocallyControlledOwner() || bOrbitKeyHeld || ShouldSuppressSessionInput())
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

void UDIVEInputComponent::HandlePrimaryActionPressed()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
	{
		return;
	}

	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

	FVector2D ScreenPosition;
	if (!TryGetCursorScreenPosition(ScreenPosition))
	{
		return;
	}

	RoutePrimaryActionPressed(ScreenPosition);
}

void UDIVEInputComponent::HandlePrimaryActionReleased()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
	{
		return;
	}

	RoutePrimaryActionReleased();
}

void UDIVEInputComponent::HandleFocusUnderCursor()
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

	FVector2D ScreenPosition;
	if (!TryGetCursorScreenPosition(ScreenPosition))
	{
		return;
	}

	if (Subsystem->FocusAtScreenPosition(ScreenPosition, PlayerController))
	{
		ApplySessionInputMode(PlayerController);
	}
}

EDIVESessionInteractionMode UDIVEInputComponent::GetInteractionMode() const
{
	if (const UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		return Subsystem->GetInteractionMode();
	}

	return EDIVESessionInteractionMode::Default;
}

void UDIVEInputComponent::SetInteractionMode(EDIVESessionInteractionMode NewMode)
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->SetInteractionMode(NewMode);
	}
}

void UDIVEInputComponent::HandleNavigateBack()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->NavigateBack();
		}
	}
}

void UDIVEInputComponent::HandleExitSession()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->EndSession();
		}
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

bool UDIVEInputComponent::ShouldSuppressSessionInput() const
{
	const UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	return Subsystem && Subsystem->IsContextMenuOpen();
}

void UDIVEInputComponent::HandleContextMenuRequested()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	ResolveComponentReferences();

	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	APlayerController* PlayerController = GetLocalPlayerController();
	if (!Subsystem || !Subsystem->IsSessionActive() || !PlayerController)
	{
		return;
	}

	FVector2D ScreenPosition;
	if (!TryGetCursorScreenPosition(ScreenPosition))
	{
		return;
	}

	if (!Subsystem->OpenContextMenuAtScreenPosition(ScreenPosition, PlayerController))
	{
		return;
	}

	WarnMissingContextMenuUIOnce();

	bOrbitKeyHeld = false;
	bPrimaryActionHeld = false;
	bPrimaryActionDragActive = false;
	bHasLastOrbitMousePosition = false;

	if (Subsystem->IsProxyDriving())
	{
		Subsystem->EndProxyDrive(false);
	}
}
