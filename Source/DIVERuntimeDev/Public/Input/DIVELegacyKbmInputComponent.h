// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"

#include "DIVELegacyKbmInputComponent.generated.h"

class UDIVEInputComponent;

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Legacy KBM Input"))
class DIVERUNTIMEDEV_API UDIVELegacyKbmInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIVELegacyKbmInputComponent();

	virtual void BeginPlay() override;

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
	void NavigateBackPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ExitSessionPressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Input")
	void ToggleIsolatePressed();

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
	bool bBindBackInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindBackInput"))
	FKey BackKey;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bBindExitInput = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (EditCondition = "bBindExitInput"))
	FKey ExitKey;

protected:
	UPROPERTY(Transient)
	TObjectPtr<UDIVEInputComponent> InputComponent;

	bool bInputBound = false;
	bool bLoggedMissingInput = false;

	void ResolveComponentReferences();
	void EnsureInputReady();
	void BindInput();
	void WarnMissingInputOnce();
};
