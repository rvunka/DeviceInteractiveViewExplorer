// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVETypes.h"

#include "DIVEGRIPBridgeComponent.generated.h"

class APlayerController;
class UDIVEInspectableComponent;
class UDIVESessionSubsystem;
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
		ToolTip = "Dive grip instance name (convention: GRIP Hand Dive). Required when the pawn has more than one UGRIPHandComponent. Empty + single Hand = legacy player-hand fallback."))
	FName GripHandComponentName = TEXT("GRIP Hand Dive");

	UPROPERTY(EditAnywhere, Category = "DIVE|GRIP", meta = (
		DisplayName = "GRIP Hand Aim Component",
		ToolTip = "Aim to suppress while driving. Empty = match Hand by replacing 'Hand' with 'Hand Aim' in the Hand name, else unique Aim if only one."))
	FName GripHandAimComponentName;

	/** Hide GRIP hand target/physics proxy spheres for the duration of a DIVE session. */
	UPROPERTY(EditAnywhere, Category = "DIVE|GRIP", meta = (
		ToolTip = "Toggles UGRIPHandComponent::bShowHandProxyVisuals while a DIVE session is active. GRIP API unchanged."))
	bool bHideGripHandProxiesDuringDiveSession = true;

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

	UFUNCTION()
	void HandleDiveSessionStarted(AActor* DeviceHost, UDIVEInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleDiveSessionEnded(EDIVESessionEndReason Reason, AActor* DeviceHost);

	UGRIPHandComponent* ResolveGripHand() const;
	UGRIPHandAimComponent* ResolveGripHandAim() const;
	APlayerController* ResolvePlayerController() const;
	bool IsLocallyControlledOwner() const;
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
	void BindDiveSessionDelegates();
	void UnbindDiveSessionDelegates();
	void SyncGripHandProxyVisibilityToDiveSession();
	UDIVESessionSubsystem* ResolveDiveSessionSubsystem() const;

	UPROPERTY(Transient)
	TObjectPtr<UGRIPHandAimComponent> CachedAimComponent;

	bool bDriving = false;
	bool bSuspendedAimUpdates = false;
	bool bManualRotateMouseCaptureActive = false;
	bool bHasPreservedCursorScreenPositionDuringRotate = false;
	bool bPreservedShowMouseCursorDuringRotate = false;
	bool bDiveSessionActive = false;
	bool bGripHandProxyVisibilitySuppressed = false;
	bool bPreservedShowHandProxyVisuals = true;
	mutable bool bLoggedLegacyHandFallback = false;
	mutable bool bLoggedAmbiguousHandResolve = false;
	FVector2D PreservedCursorScreenPositionDuringRotate = FVector2D::ZeroVector;
	EMouseCaptureMode PreservedMouseCaptureModeDuringRotate = EMouseCaptureMode::CapturePermanently;
	float GrabHoldDistance = 0.f;
};
