// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DIVEConvention.h"
#include "DIVEProxyDrive.h"
#include "DIVETypes.h"
#include "DIVESessionSubsystem.generated.h"

class ADIVECameraRig;
class UDIVEInspectableComponent;
class UPrimitiveComponent;
class AActor;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDIVESessionStarted, AActor*, DeviceHost, UDIVEInspectableComponent*, Inspectable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDIVESessionEnded, EDIVESessionEndReason, Reason, AActor*, DeviceHost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVEFocusChanged, const FDIVEFocusTarget&, FocusTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVEContextMenuVisibilityChanged, bool, bIsOpen);

UCLASS()
class DIVERUNTIME_API UDIVESessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsSessionActive() const { return SessionState == EDIVESessionState::Active; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	AActor* GetActiveDeviceHost() const { return ActiveDeviceHost.Get(); }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	UDIVEInspectableComponent* GetActiveInspectable() const { return ActiveInspectable.Get(); }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	FDIVEFocusTarget GetFocusedTarget() const { return FocusedTarget; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsIsolationActive() const { return bIsolationActive; }

	UFUNCTION(BlueprintPure, Category = "DIVE|ProxyDrive")
	bool IsProxyDriving() const { return bProxyDriving; }

	UFUNCTION(BlueprintPure, Category = "DIVE|Session")
	EDIVESessionInteractionMode GetInteractionMode() const { return InteractionMode; }

	UFUNCTION(BlueprintCallable, Category = "DIVE|Session")
	void SetInteractionMode(EDIVESessionInteractionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool TryBeginSession(AActor* DeviceHost, UDIVEInspectableComponent* Inspectable, const FDIVESessionParams& Params);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void EndSession(EDIVESessionEndReason Reason = EDIVESessionEndReason::UserExit);

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Session")
	FOnDIVESessionStarted OnSessionStarted;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Session")
	FOnDIVESessionEnded OnSessionEnded;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Session")
	FOnDIVEFocusChanged OnFocusChanged;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|ContextMenu")
	FOnDIVEContextMenuVisibilityChanged OnContextMenuVisibilityChanged;

	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu")
	bool BuildContextMenuEntries(
		const FVector2D& ScreenPosition,
		APlayerController* PlayerController,
		TArray<FDIVEContextMenuEntry>& OutEntries,
		FDIVEFocusTarget& OutPickTarget) const;

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyOrbitInput(const FVector2D& Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyZoomInput(float Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ConfigureActiveCameraInput(float OrbitSensitivity, float ZoomSensitivity);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyCameraInputFromInspectable();

	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (DeprecatedFunction, DeprecationMessage = "Use FocusAtScreenPosition"))
	bool SelectAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool FocusAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool FocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack = true);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool FocusAnchor(FName PartId);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool NavigateBack();

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool CanNavigateBack() const { return FocusStack.Num() > 1; }

	UFUNCTION(BlueprintPure, Category = "DIVE|ContextMenu")
	bool IsContextMenuOpen() const { return bContextMenuOpen; }

	/** Opens the menu at the screen position. Returns true if the menu is open after the call (false if toggled closed or build failed). */
	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu")
	bool OpenContextMenuAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu")
	bool ExecuteContextMenuAction(FName ActionId);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu")
	void CloseContextMenu();

	const TArray<FDIVEContextMenuEntry>& GetContextMenuEntries() const { return ContextMenuEntries; }

	FVector2D GetContextMenuScreenPosition() const { return ContextMenuScreenPosition; }

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool ToggleIsolateFocused();

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ClearIsolation();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Operations")
	void GetAvailableOperations(TArray<FDIVEOperationDescriptor>& OutOperations) const;

	UFUNCTION(BlueprintCallable, Category = "DIVE|Operations")
	bool ValidateFocusedOperation(FName OperationId, FText& OutFailureMessage) const;

	UFUNCTION(BlueprintCallable, Category = "DIVE|Operations")
	bool RequestFocusedOperation(FName OperationId, FDIVEOperationResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool TryBeginProxyDriveAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	void UpdateProxyDrive(const FVector2D& ScreenDelta);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	void EndProxyDrive(bool bCommit);

private:
	EDIVESessionState SessionState = EDIVESessionState::Inactive;

	TWeakObjectPtr<AActor> ActiveDeviceHost;
	TWeakObjectPtr<UDIVEInspectableComponent> ActiveInspectable;
	TWeakObjectPtr<ADIVECameraRig> ActiveCameraRig;
	TWeakObjectPtr<AActor> PreviousViewTarget;

	FDIVEFocusTarget FocusedTarget = FDIVEFocusTarget::MakeDeviceRoot();
	TArray<FDIVEFocusTarget> FocusStack;

	bool bIsolationActive = false;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> IsolatedHiddenPrimitives;
	TArray<TWeakObjectPtr<AActor>> WorldDimHiddenActors;

	bool bProxyDriving = false;
	TWeakInterfacePtr<IDIVEProxyDrive> ActiveProxyDrive;

	EDIVESessionInteractionMode InteractionMode = EDIVESessionInteractionMode::Default;

	float SessionDefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	bool bContextMenuOpen = false;
	FDIVEFocusTarget ContextMenuPickTarget;
	TArray<FDIVEContextMenuEntry> ContextMenuEntries;
	FVector2D ContextMenuScreenPosition = FVector2D::ZeroVector;

	bool ResolveFocusAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController, FDIVEFocusTarget& OutTarget) const;
	bool ApplyFocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack, bool bBlendCamera = true, bool bUseDefaultOrbitDistance = false);
	bool ApplyInitialSessionFocus(FName InitialFocusId);
	bool ApplyIsolation();
	void CollectIsolationVisiblePrimitives(const FDIVEFocusTarget& Target, TArray<UPrimitiveComponent*>& OutVisible) const;
	void ApplyWorldDim();
	void ClearWorldDim();
	void ClearProxyDrive();
};
