// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEConvention.h"
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
	void SetOrbitDistanceLimits(float InMinOrbitDistanceCm, float InMaxOrbitDistanceCm);
	void SetZoomDistanceScaling(bool bInScaleWithOrbitDistance, float InReferenceDistanceCm);
	/** Soft near-plane while focusing a part: effective min = max(MinOrbitDistance, ClearanceRadiusCm). */
	void SetFocusClearanceRadius(float ClearanceRadiusCm);
	void SetOrbitTarget(const FVector& WorldLocation, bool bRefreshTransform = false);
	void SetOrbitDistance(float Distance, bool bRefreshTransform = false);
	void SetAnchorViewpoint(const FVector& WorldLocation, const FRotator& WorldRotation, bool bRefreshTransform = false);
	void ApplyFocusPresentation(float BlendDuration);
	void ApplyOrbitDelta(const FVector2D& Delta);
	void ApplyZoomDelta(float Delta);
	void SyncOrbitFromCurrentView();
	void SyncOrbitOrientationFromCurrentView();

	float GetOrbitDistance() const { return OrbitDistance; }
	float GetMinOrbitDistance() const { return MinOrbitDistance; }
	float GetMaxOrbitDistance() const { return MaxOrbitDistance; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DIVE")
	TObjectPtr<UCameraComponent> CameraComponent;

private:
	static constexpr float MaxPitchDegrees = 89.f;
	static constexpr float MinZoomScaleFactor = 0.08f;
	static constexpr float MaxZoomScaleFactor = 4.f;

	EDIVECameraRigMode Mode = EDIVECameraRigMode::Orbit;

	float OrbitInputScale = 1.5f;
	float ZoomInputScale = 25.f;
	float MinOrbitDistance = DIVE::kDefaultMinOrbitDistance;
	float MaxOrbitDistance = DIVE::kDefaultMaxOrbitDistance;
	float FocusClearanceRadius = 0.f;
	bool bScaleZoomWithOrbitDistance = true;
	float ZoomDistanceReferenceCm = DIVE::kDefaultZoomDistanceReference;

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

	float GetEffectiveMinOrbitDistance() const;
	float ComputeZoomStepCm() const;
	static void ApplyFreeLookDelta(FQuat& Orientation, float YawRadians, float PitchRadians);
	void ClampOrientationPitch(FQuat& Orientation) const;
	void ComputeDesiredTransform(FVector& OutLocation, FQuat& OutRotation) const;
	void RefreshCameraTransform();
	void CancelFocusBlend();
};
