// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DIVECameraRig.generated.h"

class UCameraComponent;

UENUM()
enum class EDIVECameraRigMode : uint8
{
	Orbit,
	AnchorViewpoint
};

UCLASS(BlueprintType)
class DIVERUNTIME_API ADIVECameraRig : public AActor
{
	GENERATED_BODY()

public:
	ADIVECameraRig();

	virtual void Tick(float DeltaTime) override;

	void SetInputSensitivity(float InOrbitSensitivity, float InZoomSensitivity);
	void SetOrbitTarget(const FVector& WorldLocation, bool bRefreshTransform = false);
	void SetOrbitDistance(float Distance, bool bRefreshTransform = false);
	void SetAnchorViewpoint(const FVector& WorldLocation, const FRotator& WorldRotation, bool bRefreshTransform = false);
	void ApplyFocusPresentation(float BlendDuration);
	void ApplyOrbitDelta(const FVector2D& Delta);
	void ApplyZoomDelta(float Delta);
	void SyncOrbitFromCurrentView();
	void SyncOrbitOrientationFromCurrentView();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DIVE")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(EditAnywhere, Category = "DIVE")
	float MinOrbitDistance = 50.f;

	UPROPERTY(EditAnywhere, Category = "DIVE")
	float MaxOrbitDistance = 2000.f;

	UPROPERTY(EditAnywhere, Category = "DIVE", meta = (ClampMin = "1.0", ClampMax = "89.0"))
	float MaxPitchDegrees = 89.f;

private:
	EDIVECameraRigMode Mode = EDIVECameraRigMode::Orbit;

	float OrbitInputScale = 1.5f;
	float ZoomInputScale = 25.f;

	FVector OrbitTarget = FVector::ZeroVector;
	FQuat OrbitOrientation = FQuat::Identity;
	float OrbitDistance = 200.f;

	FVector AnchorViewLocation = FVector::ZeroVector;
	FQuat AnchorViewOrientation = FQuat::Identity;

	bool bFocusBlending = false;
	float FocusBlendElapsed = 0.f;
	float FocusBlendDuration = 0.f;
	FVector BlendStartLocation = FVector::ZeroVector;
	FQuat BlendStartRotation = FQuat::Identity;
	FVector BlendTargetLocation = FVector::ZeroVector;
	FQuat BlendTargetRotation = FQuat::Identity;

	static void ApplyFreeLookDelta(FQuat& Orientation, float YawRadians, float PitchRadians);
	void ClampOrientationPitch(FQuat& Orientation) const;
	void ComputeDesiredTransform(FVector& OutLocation, FQuat& OutRotation) const;
	void RefreshCameraTransform();
	void CancelFocusBlend();
};