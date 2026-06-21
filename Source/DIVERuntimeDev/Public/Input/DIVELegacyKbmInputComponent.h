// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"

#include "DIVELegacyKbmInputComponent.generated.h"

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Legacy KBM Input"))
class DIVERUNTIMEDEV_API UDIVELegacyKbmInputComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIVELegacyKbmInputComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

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

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bShowMouseCursorInSession = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bIgnoreMoveInputInSession = true;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input")
	bool bOverrideCameraSensitivity = false;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (ClampMin = "0.01", EditCondition = "bOverrideCameraSensitivity"))
	float OrbitSensitivity = 2.5f;

	UPROPERTY(EditAnywhere, Category = "DIVE|Input", meta = (ClampMin = "0.01", EditCondition = "bOverrideCameraSensitivity"))
	float ZoomSensitivity = 40.f;

protected:
	bool bInputBound = false;
	bool bOrbitKeyHeld = false;
	bool bSessionPresentationActive = false;
	bool bApplyPresentationNextTick = false;
	bool bHasLastOrbitMousePosition = false;
	FVector2D LastOrbitMousePosition = FVector2D::ZeroVector;

	void BindInput();
	class UDIVESessionSubsystem* GetSessionSubsystem() const;
	class APlayerController* GetLocalPlayerController() const;
	void UpdateSessionPresentation();
	void ApplySessionInputMode(APlayerController* PlayerController);
	void MaintainSessionInputFlags(APlayerController* PlayerController);
	void ClearSessionPresentation(APlayerController* PlayerController);
	void ApplyOrbitFromMouseDelta();
};
