// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"

#include "DIVELegacyKbmInputComponent.generated.h"

class UDIVEInputComponent;
class UInputComponent;

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Legacy KBM Input"))
class DIVERUNTIMEDEV_API UDIVELegacyKbmInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIVELegacyKbmInputComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void OrbitPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void OrbitReleased();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ZoomIn();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ZoomOut();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void SelectPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void SelectReleased();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void FocusUnderCursorPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void CycleInteractionModePressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void OperationExecutePressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void OperationExecuteReleased();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void NavigateBackPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ExitSessionPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ToggleIsolatePressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ContextMenuPressed();

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (
		DisplayName = "Input Component",
		ToolTip = "Leave empty to auto-find DIVE Input on the owner. Requires UDIVEInputComponent on the pawn."))
	FName InputComponentName;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindOrbitInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindOrbitInput"))
	FKey OrbitKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindZoomInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindZoomInput"))
	FKey ZoomInKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindZoomInput"))
	FKey ZoomOutKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindSelectInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindSelectInput"))
	FKey SelectKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindFocusUnderCursorInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindFocusUnderCursorInput"))
	FKey FocusUnderCursorKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindInteractionModeCycleInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindInteractionModeCycleInput"))
	FKey InteractionModeCycleKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindOperationExecuteInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindOperationExecuteInput"))
	FKey OperationExecuteKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindCameraUndoInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindCameraUndoInput"))
	FKey CameraUndoKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindExitInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindExitInput"))
	FKey ExitKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindIsolateInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindIsolateInput"))
	FKey IsolateKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindContextMenuInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindContextMenuInput"))
	FKey ContextMenuKey;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UDIVEInputComponent> InputComponent;

	UPROPERTY(Transient)
	TObjectPtr<UInputComponent> LegacyInputComponent;

	bool bInputBound = false;
	bool bLoggedMissingInput = false;

	void ResolveComponentReferences();
	void EnsureInputReady();
	void BindInput();
	void UnbindInput();
	void WarnMissingInputOnce();
};
