// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVETypes.h"
#include "Engine/EngineTypes.h"
#include "Math/Vector2D.h"
#include "UI/DIVESessionChromeStyle.h"

#include "DIVEInputComponent.generated.h"

class APlayerController;
class UDIVEContextMenuUIComponent;
class UDIVESessionChromeWidget;

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Input"))
class DIVERUNTIME_API UDIVEInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIVEInputComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (
		DisplayName = "Context Menu UI Component",
		ToolTip = "Leave empty to auto-find DIVE Context Menu UI on the owner. Otherwise enter the component name from the Components tab."))
	FName ContextMenuUIComponentName;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bShowMouseCursorInSession = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Session Chrome")
	bool bShowSessionChrome = true;

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

protected:
	UFUNCTION()
	void HandleSessionStarted(AActor* DeviceHost, class UDIVEInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleSessionEnded(EDIVESessionEndReason Reason, AActor* DeviceHost);

	UFUNCTION()
	void HandleInteractionModeChanged(EDIVESessionInteractionMode NewMode);

	void BindSessionDelegates();
	void UnbindSessionDelegates();
	void BeginSessionPresentation();
	void ResolveComponentReferences();
	void WarnMissingContextMenuUIOnce();
	void ShowSessionChrome();
	void HideSessionChrome();
	void UpdateSessionChromeMode(EDIVESessionInteractionMode NewMode);
	void ApplyPrimaryActionDragFromMouse();
	bool TryGetCursorScreenPosition(FVector2D& OutScreenPosition) const;
	void RoutePrimaryActionPressed(const FVector2D& ScreenPosition);
	void RoutePrimaryActionReleased();
	bool ShouldSuppressSessionInput() const;

	UPROPERTY(Transient)
	TObjectPtr<UDIVEContextMenuUIComponent> ContextMenuUIComponent;

	UPROPERTY(Transient)
	TObjectPtr<UDIVESessionChromeWidget> SessionChromeWidget;

	bool bLoggedMissingContextMenuUI = false;
	bool bOrbitKeyHeld = false;
	bool bPrimaryActionHeld = false;
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

	bool bHasPreservedInputState = false;
	bool bSessionAppliedInputFlags = false;
	bool bPreservedShowMouseCursor = false;
	bool bPreservedEnableClickEvents = false;
	bool bPreservedEnableMouseOverEvents = false;
	bool bPreservedUsedGameAndUI = false;
	EMouseCaptureMode PreservedMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
	EMouseLockMode PreservedMouseLockMode = EMouseLockMode::LockOnCapture;
};
