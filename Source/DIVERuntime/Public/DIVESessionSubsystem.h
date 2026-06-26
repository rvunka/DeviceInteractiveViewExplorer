// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DIVEConvention.h"
#include "DIVETypes.h"
#include "DIVESessionSubsystem.generated.h"

class ADIVECameraRig;
class UDIVEAnchorComponent;
class UDIVEInspectableComponent;
class UPrimitiveComponent;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDIVESessionStarted, AActor*, DeviceHost, UDIVEInspectableComponent*, Inspectable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDIVESessionEnded, EDIVESessionEndReason, Reason, AActor*, DeviceHost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVEFocusChanged, const FDIVEFocusTarget&, FocusTarget);

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

	UFUNCTION(BlueprintPure, Category = "DIVE|Manipulator")
	bool IsManipulatorDragging() const { return bManipulatorDragging; }

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

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyOrbitInput(const FVector2D& Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyZoomInput(float Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ConfigureActiveCameraInput(float OrbitSensitivity, float ZoomSensitivity);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyCameraInputFromInspectable();

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool SelectAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool FocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack = true);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool FocusAnchor(FName PartId);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool NavigateBack();

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

	UFUNCTION(BlueprintCallable, Category = "DIVE|Manipulator")
	bool TryBeginManipulatorDragAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Manipulator")
	void UpdateManipulatorDrag(const FVector2D& ScreenDelta);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Manipulator")
	void EndManipulatorDrag();

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

	bool bManipulatorDragging = false;
	TWeakObjectPtr<UDIVEAnchorComponent> ActiveManipulatorAnchor;

	float SessionDefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	bool ResolveFocusAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController, FDIVEFocusTarget& OutTarget) const;
	bool ApplyFocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack, bool bBlendCamera = true, bool bUseDefaultOrbitDistance = false);
	bool ApplyInitialSessionFocus(FName InitialFocusId);
	bool ApplyIsolation();
	void CollectIsolationVisiblePrimitives(const FDIVEFocusTarget& Target, TArray<UPrimitiveComponent*>& OutVisible) const;
	void ApplyWorldDim();
	void ClearWorldDim();
	UDIVEAnchorComponent* GetFocusedManipulatorAnchor() const;
	bool BeginManipulatorDrag();
};
