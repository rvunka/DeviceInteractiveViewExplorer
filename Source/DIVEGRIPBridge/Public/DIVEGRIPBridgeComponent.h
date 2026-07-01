// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVEPawnPhysicalDrive.h"

#include "DIVEGRIPBridgeComponent.generated.h"

class APlayerController;
class UGRIPHandAimComponent;
class UGRIPHandComponent;

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE GRIP Bridge"))
class DIVEGRIPBRIDGE_API UDIVEGRIPBridgeComponent : public UActorComponent, public IDIVEPawnPhysicalDrive
{
	GENERATED_BODY()

public:
	UDIVEGRIPBridgeComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditAnywhere, Category = "DIVE|GRIP", meta = (
		DisplayName = "GRIP Hand Component",
		ToolTip = "On this pawn. Leave empty to auto-find UGRIPHandComponent."))
	FName GripHandComponentName;

	virtual bool CanBeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) const override;
	virtual bool BeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) override;
	virtual void ApplyPawnPhysicalDriveDelta_Implementation(FVector2D ScreenDelta) override;
	virtual void EndPawnPhysicalDrive_Implementation(bool bCommit) override;
	virtual void HandlePawnPhysicalManualRotatePressed_Implementation() override;
	virtual void HandlePawnPhysicalManualRotateReleased_Implementation() override;

	/** RuntimeDev wheel routing: adjusts bridge grab depth while cursor-driven drag is active. */
	void ApplyGrabHoldDistanceScroll(float WheelDelta);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UGRIPHandComponent* ResolveGripHand() const;
	APlayerController* ResolvePlayerController() const;
	bool TryGetCursorScreenPosition(FVector2D& OutScreenPosition) const;
	bool ResolveHandTargetFromScreen(const FVector2D& ScreenPosition, FVector& OutWorldLocation) const;
	void UpdateHandTargetFromCursor();
	void ApplyManualRotationFromMouse();
	bool TryEnterManualRotateMouseCapture();
	void ExitManualRotateMouseCapture();
	void ConfigureHandDriveTickOrder();
	void SuspendGripAimUpdates();
	void RestoreGripAimUpdates();
	void SetDriveTickEnabled(bool bEnabled);

	UPROPERTY(Transient)
	TObjectPtr<UGRIPHandAimComponent> CachedAimComponent;

	bool bDriving = false;
	bool bSuspendedAimUpdates = false;
	bool bManualRotateMouseCaptureActive = false;
	bool bHasPreservedCursorScreenPositionDuringRotate = false;
	bool bPreservedShowMouseCursorDuringRotate = false;
	FVector2D PreservedCursorScreenPositionDuringRotate = FVector2D::ZeroVector;
	EMouseCaptureMode PreservedMouseCaptureModeDuringRotate = EMouseCaptureMode::CapturePermanently;
	float GrabHoldDistance = 0.f;
};
