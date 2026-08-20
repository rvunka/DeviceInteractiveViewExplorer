// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVEDeviceAction.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVEPawnPhysicalDriveProvider.h"
#include "DIVETypes.h"
#include "Engine/EngineTypes.h"
#include "Math/Vector2D.h"
#include "UI/DIVEContextMenuStyle.h"
#include "UI/DIVESessionChromeStyle.h"
#include "UI/DIVESessionChromeWidget.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "DIVEPlayerComponent.generated.h"

class APlayerController;
class UDIVEContextMenuWidget;
class UDIVEValueReadoutWidget;

/** PC does not expose the previous FInputMode; restore approximates GameOnly vs GameAndUI. */
UENUM()
enum class EDIVEPreservedInputMode : uint8
{
	GameOnly,
	GameAndUI
};

/**
 * Sole player-side DIVE ActorComponent: session input, chrome, context menu, optional Physical provider.
 */
UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Player"))
class DIVERUNTIME_API UDIVEPlayerComponent : public UActorComponent, public IDIVEPawnPhysicalDrive
{
	GENERATED_BODY()

public:
	UDIVEPlayerComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleOrbitPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleOrbitReleased();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleOrbitDelta(FVector2D Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleZoomIn();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleZoomOut();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandlePrimaryActionPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandlePrimaryActionReleased();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleManualRotatePressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleManualRotateReleased();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleFocusUnderCursor();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleNavigateBack();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleExitSession();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleToggleIsolate();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleContextMenuRequested();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void SetInteractionMode(EDIVESessionInteractionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void HandleCycleInteractionMode();

	UFUNCTION(BlueprintPure, Category = "DIVE|Input")
	EDIVESessionInteractionMode GetInteractionMode() const;

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ReapplySessionInputMode();

	UFUNCTION(BlueprintPure, Category = "DIVE")
	UDIVEPawnPhysicalDriveProvider* GetPhysicalDriveProvider() const { return PhysicalDriveProvider; }

	virtual bool CanBeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) const override;
	virtual bool BeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) override;
	virtual void EndPawnPhysicalDrive_Implementation(bool bCommit) override;
	virtual void HandlePawnPhysicalManualRotatePressed_Implementation() override;
	virtual void HandlePawnPhysicalManualRotateReleased_Implementation() override;
	virtual void HandlePawnPhysicalGrabHoldDistanceScroll_Implementation(float WheelDelta) override;

	UPROPERTY(EditAnywhere, Instanced, Category = "DIVE|Player", meta = (
		DisplayName = "Physical Drive Provider",
		ToolTip = "Optional instanced backend (DIVE GRIP when DIVEGRIPBridge is enabled). Empty + Auto Create uses the class registered by the sibling plugin."))
	TObjectPtr<UDIVEPawnPhysicalDriveProvider> PhysicalDriveProvider;

	UPROPERTY(EditAnywhere, Category = "DIVE|Player", meta = (
		DisplayName = "Auto Create Physical Drive Provider"))
	bool bAutoCreatePhysicalDriveProvider = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bShowMouseCursorInSession = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Session Chrome")
	bool bShowSessionChrome = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Session Chrome", meta = (
		EditCondition = "bShowSessionChrome",
		DisplayName = "Session Chrome Widget Class"))
	TSubclassOf<UDIVESessionChromeWidget> SessionChromeWidgetClass;

	UPROPERTY(EditAnywhere, Category = "DIVE|Session Chrome", meta = (EditCondition = "bShowSessionChrome"))
	int32 ChromeViewportZOrder = 10;

	UPROPERTY(EditAnywhere, Category = "DIVE|Session Chrome", meta = (EditCondition = "bShowSessionChrome"))
	FDIVESessionChromeStyle ChromeStyle;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bIgnoreMoveInputInSession = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bOverrideCameraSensitivity = false;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (ClampMin = "0.01", EditCondition = "bOverrideCameraSensitivity"))
	float OrbitSensitivity = 2.5f;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (ClampMin = "0.01", EditCondition = "bOverrideCameraSensitivity"))
	float ZoomSensitivity = 40.f;

	UPROPERTY(EditAnywhere, Category = "DIVE|ContextMenu")
	int32 ViewportZOrder = 20;

	UPROPERTY(EditAnywhere, Category = "DIVE|ContextMenu")
	FDIVEContextMenuStyle MenuStyle;

	UPROPERTY(EditAnywhere, Category = "DIVE|ContextMenu", meta = (
		DisplayName = "Context Menu Widget Class"))
	TSubclassOf<UDIVEContextMenuWidget> ContextMenuWidgetClass;

	UPROPERTY(EditAnywhere, Category = "DIVE|Action|HUD", meta = (
		DisplayName = "Value Readout Widget Class"))
	TSubclassOf<UDIVEValueReadoutWidget> ValueReadoutWidgetClass;

	UPROPERTY(EditAnywhere, Category = "DIVE|Action|HUD")
	int32 ValueReadoutZOrder = 15;

protected:
	UFUNCTION()
	void HandleSessionStarted(AActor* DeviceHost, class UDIVEInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleSessionEnded(EDIVESessionEndReason Reason, AActor* DeviceHost);

	UFUNCTION()
	void HandleInteractionModeChanged(EDIVESessionInteractionMode NewMode);

	UFUNCTION()
	void HandleContextMenuVisibilityChanged(bool bIsOpen);

	UFUNCTION()
	void HandleContextMenuEntrySelected(UDIVEDeviceAction* Action, FName TargetKey, FName BindingId);

	UFUNCTION()
	void HandleContextMenuDismissed();

	UFUNCTION()
	void HandleInteractionValueChanged(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		const FDIVEInteractionValue& Value);

	void BindSessionDelegates();
	void UnbindSessionDelegates();
	void BeginSessionPresentation();
	void ShowSessionChrome();
	void HideSessionChrome();
	void UpdateSessionChromeMode(EDIVESessionInteractionMode NewMode);
	void ApplyPrimaryActionDragFromMouse();
	bool TryGetCursorScreenPosition(FVector2D& OutScreenPosition) const;
	void RoutePrimaryActionPressed(const FVector2D& ScreenPosition);
	void RoutePrimaryActionReleased();
	bool ShouldSuppressSessionInput() const;
	void ShowContextMenu();
	void HideContextMenu();
	void EnsureValueReadoutWidget();
	void HideValueReadout();

	UPROPERTY(Transient)
	TObjectPtr<UDIVESessionChromeWidget> SessionChromeWidget;

	UPROPERTY(Transient)
	TObjectPtr<UDIVEContextMenuWidget> ContextMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UDIVEValueReadoutWidget> ValueReadoutWidget;

	bool bOrbitKeyHeld = false;
	FVector2D PrimaryActionLastPosition = FVector2D::ZeroVector;
	bool bSessionPresentationActive = false;
	bool bHasLastOrbitMousePosition = false;
	FVector2D LastOrbitMousePosition = FVector2D::ZeroVector;

	class UDIVESessionSubsystem* GetSessionSubsystem() const;
	class APlayerController* GetLocalPlayerController() const;
	bool IsLocallyControlledOwner() const;
	void CapturePreSessionInputState(APlayerController* PlayerController);
	void RestorePreSessionInputState(APlayerController* PlayerController);
	void ApplySessionInputMode(APlayerController* PlayerController);
	void MaintainSessionInputFlags(APlayerController* PlayerController);
	void ClearSessionPresentation(APlayerController* PlayerController);
	void ApplyOrbitFromMouseDelta();
	void RefreshLocalControlState();
	void HandleGainedLocalControl();
	void HandleLostLocalControl();

	bool bHasPreservedInputState = false;
	bool bSessionAppliedInputFlags = false;
	bool bPreservedShowMouseCursor = false;
	bool bPreservedEnableClickEvents = false;
	bool bPreservedEnableMouseOverEvents = false;
	EDIVEPreservedInputMode PreservedInputMode = EDIVEPreservedInputMode::GameOnly;
	EMouseCaptureMode PreservedMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
	EMouseLockMode PreservedMouseLockMode = EMouseLockMode::LockOnCapture;
	bool bWasLocallyControlled = false;
	bool bHasLocalControlSample = false;

	TWeakObjectPtr<APlayerController> SessionPresentationController;
};
