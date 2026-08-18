// Copyright (c) 2026. All Rights Reserved.

#include "DIVEPlayerComponent.h"

#include "DIVEInspectableComponent.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVEDeviceAction.h"
#include "DIVESessionSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/Layout/Anchors.h"
#include "UI/DIVEContextMenuWidget.h"
#include "UI/DIVESessionChromeWidget.h"
#include "UI/DIVEValueReadoutWidget.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
constexpr float IdleLocalControlTickInterval = 0.15f;

UObject* PhysicalDriveInterfaceObject(UDIVEPawnPhysicalDriveProvider* Provider)
{
	return (Provider && Provider->Implements<UDIVEPawnPhysicalDrive>()) ? Provider : nullptr;
}
}

UDIVEPlayerComponent::UDIVEPlayerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickInterval = IdleLocalControlTickInterval;
	ValueReadoutWidgetClass = UDIVEValueReadoutWidget::StaticClass();
}

void UDIVEPlayerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!PhysicalDriveProvider && bAutoCreatePhysicalDriveProvider)
	{
		if (UClass* ProviderClass = DIVEPhysicalDriveExtension::GetRegisteredProviderClass())
		{
			PhysicalDriveProvider = NewObject<UDIVEPawnPhysicalDriveProvider>(
				this,
				ProviderClass,
				TEXT("PhysicalDriveProvider"));
		}
	}

	if (PhysicalDriveProvider)
	{
		PhysicalDriveProvider->InitializeOnPawn(GetOwner());
	}

	SetComponentTickEnabled(true);
	bHasLocalControlSample = false;
	BindSessionDelegates();
	RefreshLocalControlState();

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive() && IsLocallyControlledOwner() && !bSessionPresentationActive)
		{
			HandleSessionStarted(Subsystem->GetActiveDeviceHost(), Subsystem->GetActiveInspectable());
		}
	}
}

void UDIVEPlayerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

	HideContextMenu();
	HideValueReadout();
	if (PhysicalDriveProvider)
	{
		PhysicalDriveProvider->ShutdownOnPawn();
	}

	SetComponentTickEnabled(false);
	bOrbitKeyHeld = false;
	bHasLastOrbitMousePosition = false;
	bHasLocalControlSample = false;
	Super::EndPlay(EndPlayReason);
}

void UDIVEPlayerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshLocalControlState();

	if (!IsLocallyControlledOwner())
	{
		return;
	}

	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();

	if (bOrbitKeyHeld)
	{
		ApplyOrbitFromMouseDelta();
	}

	if (Subsystem && Subsystem->IsProxyDriving() && !Subsystem->IsPawnPhysicalDriveActive() && !ShouldSuppressSessionInput())
	{
		ApplyPrimaryActionDragFromMouse();
	}

	if (Subsystem
		&& Subsystem->IsSessionActive()
		&& Subsystem->GetInteractionMode() == EDIVESessionInteractionMode::Default
		&& !ShouldSuppressSessionInput()
		&& !Subsystem->IsProxyDriving())
	{
		UDIVEInspectableComponent* Inspectable = Subsystem->GetActiveInspectable();
		if (!Inspectable || !Inspectable->HasPickHoverOverlay())
		{
			Subsystem->ClearPickHover();
		}
		else
		{
			FVector2D ScreenPosition;
			if (TryGetCursorScreenPosition(ScreenPosition))
			{
				Subsystem->UpdatePickHover(ScreenPosition, GetLocalPlayerController());
			}
		}
	}
	else if (Subsystem)
	{
		Subsystem->ClearPickHover();
	}

	if (PhysicalDriveProvider)
	{
		PhysicalDriveProvider->TickDrive(DeltaTime);
	}
}

void UDIVEPlayerComponent::BindSessionDelegates()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->OnSessionStarted.AddUniqueDynamic(this, &UDIVEPlayerComponent::HandleSessionStarted);
		Subsystem->OnSessionEnded.AddUniqueDynamic(this, &UDIVEPlayerComponent::HandleSessionEnded);
		Subsystem->OnInteractionModeChanged.AddUniqueDynamic(this, &UDIVEPlayerComponent::HandleInteractionModeChanged);
		Subsystem->OnContextMenuVisibilityChanged.AddUniqueDynamic(this, &UDIVEPlayerComponent::HandleContextMenuVisibilityChanged);
		Subsystem->OnInteractionValueChanged.AddUniqueDynamic(this, &UDIVEPlayerComponent::HandleInteractionValueChanged);

		if (Subsystem->IsContextMenuOpen())
		{
			HandleContextMenuVisibilityChanged(true);
		}
	}
}

void UDIVEPlayerComponent::UnbindSessionDelegates()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->OnSessionStarted.RemoveDynamic(this, &UDIVEPlayerComponent::HandleSessionStarted);
		Subsystem->OnSessionEnded.RemoveDynamic(this, &UDIVEPlayerComponent::HandleSessionEnded);
		Subsystem->OnInteractionModeChanged.RemoveDynamic(this, &UDIVEPlayerComponent::HandleInteractionModeChanged);
		Subsystem->OnContextMenuVisibilityChanged.RemoveDynamic(this, &UDIVEPlayerComponent::HandleContextMenuVisibilityChanged);
		Subsystem->OnInteractionValueChanged.RemoveDynamic(this, &UDIVEPlayerComponent::HandleInteractionValueChanged);
	}
}

void UDIVEPlayerComponent::HandleSessionStarted(AActor* /*DeviceHost*/, UDIVEInspectableComponent* /*Inspectable*/)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	BeginSessionPresentation();
}

void UDIVEPlayerComponent::HandleSessionEnded(EDIVESessionEndReason /*Reason*/, AActor* /*DeviceHost*/)
{
	APlayerController* PlayerController = SessionPresentationController.Get();
	if (!PlayerController)
	{
		PlayerController = GetLocalPlayerController();
	}

	ClearSessionPresentation(PlayerController);
	bOrbitKeyHeld = false;
	bHasLastOrbitMousePosition = false;
	HideContextMenu();
	HideValueReadout();
}

void UDIVEPlayerComponent::RefreshLocalControlState()
{
	const bool bNowLocal = IsLocallyControlledOwner();
	if (!bHasLocalControlSample)
	{
		bHasLocalControlSample = true;
		bWasLocallyControlled = bNowLocal;
		if (bNowLocal)
		{
			HandleGainedLocalControl();
		}
		return;
	}

	if (bNowLocal == bWasLocallyControlled)
	{
		return;
	}

	bWasLocallyControlled = bNowLocal;
	if (bNowLocal)
	{
		HandleGainedLocalControl();
	}
	else
	{
		HandleLostLocalControl();
	}
}

void UDIVEPlayerComponent::HandleGainedLocalControl()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (Subsystem && Subsystem->IsSessionActive() && !bSessionPresentationActive)
	{
		BeginSessionPresentation();
	}
}

void UDIVEPlayerComponent::HandleLostLocalControl()
{
	APlayerController* PlayerController = SessionPresentationController.Get();
	if (!PlayerController)
	{
		PlayerController = GetLocalPlayerController();
	}

	ClearSessionPresentation(PlayerController);

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive())
		{
			Subsystem->EndSession(EDIVESessionEndReason::Forced);
		}
	}
}

void UDIVEPlayerComponent::BeginSessionPresentation()
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

	Subsystem->ApplyCameraInputFromInspectable();
	if (bOverrideCameraSensitivity)
	{
		Subsystem->ConfigureActiveCameraInput(OrbitSensitivity, ZoomSensitivity);
	}

	bSessionPresentationActive = true;
	SessionPresentationController = PlayerController;
	PrimaryComponentTick.TickInterval = 0.f;
	ShowSessionChrome();
}

UDIVESessionSubsystem* UDIVEPlayerComponent::GetSessionSubsystem() const
{
	if (UWorld* World = GetWorld())
	{
		return World->GetSubsystem<UDIVESessionSubsystem>();
	}

	return nullptr;
}

APlayerController* UDIVEPlayerComponent::GetLocalPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
}

bool UDIVEPlayerComponent::IsLocallyControlledOwner() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

void UDIVEPlayerComponent::ApplyPrimaryActionDragFromMouse()
{
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem || !Subsystem->IsProxyDriving())
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
		Subsystem->UpdateActiveInteraction(FVector2D(MouseDeltaX, MouseDeltaY));
		return;
	}

	FVector2D CurrentPosition;
	if (!TryGetCursorScreenPosition(CurrentPosition))
	{
		return;
	}

	const FVector2D Delta = CurrentPosition - PrimaryActionLastPosition;
	PrimaryActionLastPosition = CurrentPosition;

	if (!Delta.IsNearlyZero())
	{
		Subsystem->UpdateActiveInteraction(Delta);
	}
}

bool UDIVEPlayerComponent::TryGetCursorScreenPosition(FVector2D& OutScreenPosition) const
{
	if (const APlayerController* PlayerController = GetLocalPlayerController())
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		if (PlayerController->GetMousePosition(MouseX, MouseY))
		{
			OutScreenPosition = FVector2D(MouseX, MouseY);
			return true;
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2D ViewportPosition;
			if (ViewportClient->GetMousePosition(ViewportPosition))
			{
				OutScreenPosition = ViewportPosition;
				return true;
			}
		}
	}

	return false;
}

void UDIVEPlayerComponent::RoutePrimaryActionPressed(const FVector2D& ScreenPosition)
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

	// Continuous / proxy already active (e.g. started from context menu): click commits/ends.
	if (Subsystem->IsProxyDriving())
	{
		Subsystem->HandleActivePawnPhysicalManualRotateReleased();
		Subsystem->EndProxyDrive(true);
		if (bSessionPresentationActive)
		{
			ApplySessionInputMode(PlayerController);
		}
		return;
	}

	if (Subsystem->GetInteractionMode() == EDIVESessionInteractionMode::Physical)
	{
		if (Subsystem->TryBeginProxyDriveAtScreenPosition(ScreenPosition, PlayerController))
		{
			if (UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
			{
				ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);
			}
			TryGetCursorScreenPosition(PrimaryActionLastPosition);
			return;
		}

		return;
	}

	if (Subsystem->GetInteractionMode() == EDIVESessionInteractionMode::Default)
	{
		Subsystem->ExecutePrimaryActionAtScreenPosition(ScreenPosition, PlayerController);
		return;
	}
}

void UDIVEPlayerComponent::RoutePrimaryActionReleased()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->ConsumeIgnoreNextPrimaryActionRelease())
		{
			TryGetCursorScreenPosition(PrimaryActionLastPosition);
			return;
		}

		const bool bWasProxyDriving = Subsystem->IsProxyDriving();
		if (bWasProxyDriving)
		{
			Subsystem->HandleActivePawnPhysicalManualRotateReleased();
			Subsystem->EndProxyDrive(true);
		}

		if (bWasProxyDriving && bSessionPresentationActive)
		{
			if (APlayerController* PlayerController = GetLocalPlayerController())
			{
				ApplySessionInputMode(PlayerController);
			}
		}
	}

}

void UDIVEPlayerComponent::CapturePreSessionInputState(APlayerController* PlayerController)
{
	if (!PlayerController || bHasPreservedInputState)
	{
		return;
	}

	bPreservedShowMouseCursor = PlayerController->bShowMouseCursor;
	bPreservedEnableClickEvents = PlayerController->bEnableClickEvents;
	bPreservedEnableMouseOverEvents = PlayerController->bEnableMouseOverEvents;
	// GameAndUI only when both cursor and click events were already on (avoids false restore after debug cursor).
	PreservedInputMode = (PlayerController->bShowMouseCursor && PlayerController->bEnableClickEvents)
		? EDIVEPreservedInputMode::GameAndUI
		: EDIVEPreservedInputMode::GameOnly;

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

void UDIVEPlayerComponent::RestorePreSessionInputState(APlayerController* PlayerController)
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

	if (PreservedInputMode == EDIVEPreservedInputMode::GameAndUI)
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

void UDIVEPlayerComponent::ApplySessionInputMode(APlayerController* PlayerController)
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

void UDIVEPlayerComponent::MaintainSessionInputFlags(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	PlayerController->bShowMouseCursor = bShowMouseCursorInSession;
}

void UDIVEPlayerComponent::ClearSessionPresentation(APlayerController* PlayerController)
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
	HideSessionChrome();
	bSessionPresentationActive = false;
	bSessionAppliedInputFlags = false;
	SessionPresentationController.Reset();
	PrimaryComponentTick.TickInterval = IdleLocalControlTickInterval;
}

void UDIVEPlayerComponent::ApplyOrbitFromMouseDelta()
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

void UDIVEPlayerComponent::HandleOrbitPressed()
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

void UDIVEPlayerComponent::HandleOrbitReleased()
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

void UDIVEPlayerComponent::HandleOrbitDelta(FVector2D Delta)
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

void UDIVEPlayerComponent::HandleZoomIn()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive() && !Subsystem->IsProxyDriving())
		{
			Subsystem->ApplyZoomInput(1.f);
		}
	}
}

void UDIVEPlayerComponent::HandleZoomOut()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsSessionActive() && !Subsystem->IsProxyDriving())
		{
			Subsystem->ApplyZoomInput(-1.f);
		}
	}
}

void UDIVEPlayerComponent::HandlePrimaryActionPressed()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (ShouldSuppressSessionInput())
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

void UDIVEPlayerComponent::HandlePrimaryActionReleased()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		if (Subsystem->IsContextMenuOpen())
		{
			Subsystem->ConsumeIgnoreNextPrimaryActionRelease();
			return;
		}
	}

	if (ShouldSuppressSessionInput())
	{
		return;
	}

	RoutePrimaryActionReleased();
}

void UDIVEPlayerComponent::HandleManualRotatePressed()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
	{
		return;
	}

	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem || !Subsystem->IsPawnPhysicalDriveActive())
	{
		return;
	}

	Subsystem->HandleActivePawnPhysicalManualRotatePressed();
}

void UDIVEPlayerComponent::HandleManualRotateReleased()
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!Subsystem)
	{
		return;
	}

	if (Subsystem->IsPawnPhysicalDriveActive())
	{
		Subsystem->HandleActivePawnPhysicalManualRotateReleased();
	}
}

void UDIVEPlayerComponent::HandleFocusUnderCursor()
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

EDIVESessionInteractionMode UDIVEPlayerComponent::GetInteractionMode() const
{
	if (const UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		return Subsystem->GetInteractionMode();
	}

	return EDIVESessionInteractionMode::Default;
}

void UDIVEPlayerComponent::ReapplySessionInputMode()
{
	if (!IsLocallyControlledOwner() || !bSessionPresentationActive)
	{
		return;
	}

	if (APlayerController* PlayerController = GetLocalPlayerController())
	{
		ApplySessionInputMode(PlayerController);
	}
}

void UDIVEPlayerComponent::HandleInteractionModeChanged(EDIVESessionInteractionMode NewMode)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	UpdateSessionChromeMode(NewMode);
}

void UDIVEPlayerComponent::ShowSessionChrome()
{
	if (!bShowSessionChrome || !bSessionPresentationActive)
	{
		return;
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!PlayerController || !Subsystem || !Subsystem->IsSessionActive())
	{
		return;
	}

	if (!SessionChromeWidget)
	{
		TSubclassOf<UDIVESessionChromeWidget> WidgetClass = SessionChromeWidgetClass;
		if (!*WidgetClass)
		{
			WidgetClass = UDIVESessionChromeWidget::StaticClass();
		}
		SessionChromeWidget = CreateWidget<UDIVESessionChromeWidget>(PlayerController, WidgetClass);
	}

	if (!SessionChromeWidget)
	{
		return;
	}

	SessionChromeWidget->SetStyle(ChromeStyle);
	SessionChromeWidget->SetInteractionMode(Subsystem->GetInteractionMode());

	if (!SessionChromeWidget->IsInViewport())
	{
		SessionChromeWidget->AddToViewport(ChromeViewportZOrder);
	}

	SessionChromeWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
	SessionChromeWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
	SessionChromeWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDIVEPlayerComponent::HideSessionChrome()
{
	if (SessionChromeWidget)
	{
		SessionChromeWidget->RemoveFromParent();
		SessionChromeWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UDIVEPlayerComponent::UpdateSessionChromeMode(EDIVESessionInteractionMode NewMode)
{
	if (SessionChromeWidget && SessionChromeWidget->IsInViewport())
	{
		SessionChromeWidget->SetInteractionMode(NewMode);
	}
}

void UDIVEPlayerComponent::SetInteractionMode(EDIVESessionInteractionMode NewMode)
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->SetInteractionMode(NewMode);
	}
}

void UDIVEPlayerComponent::HandleCycleInteractionMode()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
	{
		return;
	}

	const EDIVESessionInteractionMode CurrentMode = GetInteractionMode();
	SetInteractionMode(
		CurrentMode == EDIVESessionInteractionMode::Default
			? EDIVESessionInteractionMode::Physical
			: EDIVESessionInteractionMode::Default);
}

void UDIVEPlayerComponent::HandleNavigateBack()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
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

void UDIVEPlayerComponent::HandleExitSession()
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

void UDIVEPlayerComponent::HandleToggleIsolate()
{
	if (!IsLocallyControlledOwner() || ShouldSuppressSessionInput())
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

bool UDIVEPlayerComponent::ShouldSuppressSessionInput() const
{
	const UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	return Subsystem && Subsystem->IsContextMenuOpen();
}

void UDIVEPlayerComponent::HandleContextMenuRequested()
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

	if (!Subsystem->OpenContextMenuAtScreenPosition(ScreenPosition, PlayerController))
	{
		return;
	}

	bOrbitKeyHeld = false;
	bHasLastOrbitMousePosition = false;

	if (Subsystem->IsProxyDriving())
	{
		Subsystem->HandleActivePawnPhysicalManualRotateReleased();
		Subsystem->EndProxyDrive(false);
	}
}

#if WITH_EDITOR
EDataValidationResult UDIVEPlayerComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return Result;
	}

	if (!Owner->IsA<APawn>())
	{
		Context.AddError(FText::FromString(TEXT(
			"DIVE Player must be owned by an APawn (locally controlled player character).")));
		Result = EDataValidationResult::Invalid;
	}

	TArray<UDIVEPlayerComponent*> Roots;
	Owner->GetComponents<UDIVEPlayerComponent>(Roots);
	if (Roots.Num() > 1)
	{
		Context.AddError(FText::FromString(TEXT(
			"Multiple DIVE Player components on the same actor — keep a single DIVE Player.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

void UDIVEPlayerComponent::HandleContextMenuVisibilityChanged(const bool bIsOpen)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (bIsOpen)
	{
		ShowContextMenu();
	}
	else
	{
		HideContextMenu();
	}
}

void UDIVEPlayerComponent::HandleContextMenuEntrySelected(
	UDIVEDeviceAction* Action,
	FName TargetKey,
	FName BindingId)
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->ExecuteContextMenuAction(Action, TargetKey, BindingId);
	}
}

void UDIVEPlayerComponent::HandleContextMenuDismissed()
{
	if (UDIVESessionSubsystem* Subsystem = GetSessionSubsystem())
	{
		Subsystem->CloseContextMenu();
	}
}

void UDIVEPlayerComponent::HandleInteractionValueChanged(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context,
	float NormalizedValue)
{
	(void)Context;
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (!Action)
	{
		if (ValueReadoutWidget)
		{
			ValueReadoutWidget->ClearReadout();
		}
		return;
	}

	EnsureValueReadoutWidget();
	if (ValueReadoutWidget)
	{
		ValueReadoutWidget->SetReadout(Action->GetResolvedDisplayName(), NormalizedValue);
	}
}

void UDIVEPlayerComponent::EnsureValueReadoutWidget()
{
	if (ValueReadoutWidget)
	{
		return;
	}

	APlayerController* PlayerController = GetLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	TSubclassOf<UDIVEValueReadoutWidget> WidgetClass = ValueReadoutWidgetClass;
	if (!*WidgetClass)
	{
		WidgetClass = UDIVEValueReadoutWidget::StaticClass();
	}

	ValueReadoutWidget = CreateWidget<UDIVEValueReadoutWidget>(PlayerController, WidgetClass);
	if (ValueReadoutWidget && !ValueReadoutWidget->IsInViewport())
	{
		ValueReadoutWidget->AddToViewport(ValueReadoutZOrder);
	}
}

void UDIVEPlayerComponent::HideValueReadout()
{
	if (ValueReadoutWidget)
	{
		ValueReadoutWidget->ClearReadout();
		ValueReadoutWidget->RemoveFromParent();
		ValueReadoutWidget = nullptr;
	}
}

void UDIVEPlayerComponent::ShowContextMenu()
{
	APlayerController* PlayerController = GetLocalPlayerController();
	UDIVESessionSubsystem* Subsystem = GetSessionSubsystem();
	if (!PlayerController || !Subsystem)
	{
		return;
	}

	if (!ContextMenuWidget)
	{
		TSubclassOf<UDIVEContextMenuWidget> WidgetClass = ContextMenuWidgetClass;
		if (!*WidgetClass)
		{
			WidgetClass = UDIVEContextMenuWidget::StaticClass();
		}
		ContextMenuWidget = CreateWidget<UDIVEContextMenuWidget>(PlayerController, WidgetClass);
		if (ContextMenuWidget)
		{
			ContextMenuWidget->OnEntrySelected.AddDynamic(this, &UDIVEPlayerComponent::HandleContextMenuEntrySelected);
			ContextMenuWidget->OnDismissed.AddDynamic(this, &UDIVEPlayerComponent::HandleContextMenuDismissed);
		}
	}

	if (!ContextMenuWidget)
	{
		return;
	}

	ContextMenuWidget->SetStyle(MenuStyle);
	ContextMenuWidget->SetEntries(Subsystem->GetContextMenuEntries());
	ContextMenuWidget->SetScreenPosition(Subsystem->GetContextMenuScreenPosition());

	if (!ContextMenuWidget->IsInViewport())
	{
		ContextMenuWidget->AddToViewport(ViewportZOrder);
	}

	ContextMenuWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
	ContextMenuWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
	ContextMenuWidget->SetVisibility(ESlateVisibility::Visible);
	ContextMenuWidget->SetKeyboardFocus();
}

void UDIVEPlayerComponent::HideContextMenu()
{
	if (ContextMenuWidget)
	{
		ContextMenuWidget->RemoveFromParent();
		ContextMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UDIVEPlayerComponent::CanBeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) const
{
	if (UObject* DriveObject = PhysicalDriveInterfaceObject(PhysicalDriveProvider))
	{
		return IDIVEPawnPhysicalDrive::Execute_CanBeginPawnPhysicalDrive(DriveObject, Context);
	}
	return false;
}

bool UDIVEPlayerComponent::BeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context)
{
	if (UObject* DriveObject = PhysicalDriveInterfaceObject(PhysicalDriveProvider))
	{
		return IDIVEPawnPhysicalDrive::Execute_BeginPawnPhysicalDrive(DriveObject, Context);
	}
	return false;
}

void UDIVEPlayerComponent::EndPawnPhysicalDrive_Implementation(const bool bCommit)
{
	if (UObject* DriveObject = PhysicalDriveInterfaceObject(PhysicalDriveProvider))
	{
		IDIVEPawnPhysicalDrive::Execute_EndPawnPhysicalDrive(DriveObject, bCommit);
	}
}

void UDIVEPlayerComponent::HandlePawnPhysicalManualRotatePressed_Implementation()
{
	if (UObject* DriveObject = PhysicalDriveInterfaceObject(PhysicalDriveProvider))
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotatePressed(DriveObject);
	}
}

void UDIVEPlayerComponent::HandlePawnPhysicalManualRotateReleased_Implementation()
{
	if (UObject* DriveObject = PhysicalDriveInterfaceObject(PhysicalDriveProvider))
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotateReleased(DriveObject);
	}
}

void UDIVEPlayerComponent::HandlePawnPhysicalGrabHoldDistanceScroll_Implementation(const float WheelDelta)
{
	if (UObject* DriveObject = PhysicalDriveInterfaceObject(PhysicalDriveProvider))
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalGrabHoldDistanceScroll(DriveObject, WheelDelta);
	}
}
