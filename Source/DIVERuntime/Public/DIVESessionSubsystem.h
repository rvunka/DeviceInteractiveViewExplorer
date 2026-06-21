// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "DIVEConvention.h"
#include "DIVETypes.h"
#include "DIVESessionSubsystem.generated.h"

class ADIVECameraRig;
class UDIVEInspectableComponent;
class UPrimitiveComponent;

UCLASS()
class DIVERUNTIME_API UDIVESessionSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
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
	FDIVEFocusTarget GetHoveredTarget() const { return HoveredTarget; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsIsolationActive() const { return bIsolationActive; }

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool TryBeginSession(AActor* DeviceHost, UDIVEInspectableComponent* Inspectable, const FDIVESessionParams& Params);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void EndSession();

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
	bool UpdateHoverAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

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

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool RequestFocusedOperation(FName OperationId, FDIVEOperationResult& OutResult);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;
	virtual bool IsTickableWhenPaused() const override { return false; }
	virtual UWorld* GetTickableGameObjectWorld() const override;

private:
	EDIVESessionState SessionState = EDIVESessionState::Inactive;

	TWeakObjectPtr<AActor> ActiveDeviceHost;
	TWeakObjectPtr<UDIVEInspectableComponent> ActiveInspectable;
	TWeakObjectPtr<ADIVECameraRig> ActiveCameraRig;
	TWeakObjectPtr<AActor> PreviousViewTarget;

	FDIVEFocusTarget FocusedTarget = FDIVEFocusTarget::MakeDeviceRoot();
	FDIVEFocusTarget HoveredTarget = FDIVEFocusTarget::MakeDeviceRoot();
	TArray<FDIVEFocusTarget> FocusStack;

	TArray<TWeakObjectPtr<UPrimitiveComponent>> HighlightedHoverPrimitives;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> HighlightedFocusPrimitives;

	bool bIsolationActive = false;
	TArray<TWeakObjectPtr<UPrimitiveComponent>> IsolatedHiddenPrimitives;

	float SessionDefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	bool ResolveFocusAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController, FDIVEFocusTarget& OutTarget) const;
	bool ApplyFocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack);
	void RefreshHighlights();
	void ClearHighlightPrimitives(TArray<TWeakObjectPtr<UPrimitiveComponent>>& Primitives);
	void ApplyHighlightForPrimitive(UPrimitiveComponent* Primitive, int32 StencilValue, TArray<TWeakObjectPtr<UPrimitiveComponent>>& OutTrackedPrimitives);
	bool ApplyIsolation();
	void CollectDevicePrimitives(TArray<UPrimitiveComponent*>& OutPrimitives) const;
	void CollectIsolationVisiblePrimitives(const FDIVEFocusTarget& Target, TArray<UPrimitiveComponent*>& OutVisible) const;
};
