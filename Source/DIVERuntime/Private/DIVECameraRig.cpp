// Copyright (c) 2026. All Rights Reserved.

#include "DIVECameraRig.h"

#include "Camera/CameraComponent.h"

namespace
{
FQuat MakeLookOrientation(const FVector& LookDirection)
{
	const FVector Forward = LookDirection.GetSafeNormal();
	if (Forward.IsNearlyZero())
	{
		return FQuat::Identity;
	}

	return Forward.Rotation().Quaternion();
}
}

ADIVECameraRig::ADIVECameraRig()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(RootComponent);

	OrbitOrientation = FRotator(-20.f, 0.f, 0.f).Quaternion();
}

void ADIVECameraRig::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!bFocusBlending)
	{
		return;
	}

	FocusBlendElapsed += DeltaTime;
	const float Alpha = FMath::Clamp(FocusBlendElapsed / FocusBlendDuration, 0.f, 1.f);
	const float SmoothedAlpha = FMath::SmoothStep(0.f, 1.f, Alpha);

	SetActorLocation(FMath::Lerp(BlendStartLocation, BlendTargetLocation, SmoothedAlpha));
	SetActorRotation(FQuat::Slerp(BlendStartRotation, BlendTargetRotation, SmoothedAlpha).GetNormalized());

	if (Alpha >= 1.f - KINDA_SMALL_NUMBER)
	{
		CancelFocusBlend();
	}
}

void ADIVECameraRig::SetInputSensitivity(float InOrbitSensitivity, float InZoomSensitivity)
{
	OrbitInputScale = FMath::Max(InOrbitSensitivity, 0.01f);
	ZoomInputScale = FMath::Max(InZoomSensitivity, 0.01f);
}

void ADIVECameraRig::SetOrbitDistanceLimits(const float InMinOrbitDistanceCm, const float InMaxOrbitDistanceCm)
{
	MinOrbitDistance = FMath::Max(InMinOrbitDistanceCm, 1.f);
	MaxOrbitDistance = FMath::Max(InMaxOrbitDistanceCm, MinOrbitDistance);
	OrbitDistance = FMath::Clamp(OrbitDistance, GetEffectiveMinOrbitDistance(), MaxOrbitDistance);
}

void ADIVECameraRig::SetZoomDistanceScaling(const bool bInScaleWithOrbitDistance, const float InReferenceDistanceCm)
{
	bScaleZoomWithOrbitDistance = bInScaleWithOrbitDistance;
	ZoomDistanceReferenceCm = FMath::Max(InReferenceDistanceCm, 1.f);
}

void ADIVECameraRig::SetFocusClearanceRadius(const float ClearanceRadiusCm)
{
	FocusClearanceRadius = FMath::Max(ClearanceRadiusCm, 0.f);
	OrbitDistance = FMath::Clamp(OrbitDistance, GetEffectiveMinOrbitDistance(), MaxOrbitDistance);
}

float ADIVECameraRig::GetEffectiveMinOrbitDistance() const
{
	return FMath::Max(MinOrbitDistance, FocusClearanceRadius);
}

float ADIVECameraRig::ComputeZoomStepCm() const
{
	if (!bScaleZoomWithOrbitDistance)
	{
		return ZoomInputScale;
	}

	const float Reference = FMath::Max(ZoomDistanceReferenceCm, 1.f);
	const float Scale = FMath::Clamp(OrbitDistance / Reference, MinZoomScaleFactor, MaxZoomScaleFactor);
	return ZoomInputScale * Scale;
}

void ADIVECameraRig::SetOrbitTarget(const FVector& WorldLocation, bool bRefreshTransform)
{
	const bool bLeaveAnchorViewpoint = Mode == EDIVECameraRigMode::AnchorViewpoint;
	Mode = EDIVECameraRigMode::Orbit;
	OrbitTarget = WorldLocation;

	if (bLeaveAnchorViewpoint)
	{
		SyncOrbitFromCurrentView();
	}

	if (bRefreshTransform)
	{
		RefreshCameraTransform();
	}
}

void ADIVECameraRig::SetOrbitDistance(float Distance, bool bRefreshTransform)
{
	OrbitDistance = FMath::Clamp(Distance, GetEffectiveMinOrbitDistance(), MaxOrbitDistance);

	if (bRefreshTransform && Mode == EDIVECameraRigMode::Orbit)
	{
		RefreshCameraTransform();
	}
}

void ADIVECameraRig::SetAnchorViewpoint(const FVector& WorldLocation, const FRotator& WorldRotation, bool bRefreshTransform)
{
	Mode = EDIVECameraRigMode::AnchorViewpoint;
	AnchorViewLocation = WorldLocation;
	AnchorViewOrientation = WorldRotation.Quaternion();

	if (bRefreshTransform)
	{
		RefreshCameraTransform();
	}
}

void ADIVECameraRig::ApplyFocusPresentation(float BlendDuration)
{
	FVector DesiredLocation;
	FQuat DesiredRotation;
	ComputeDesiredTransform(DesiredLocation, DesiredRotation);

	if (BlendDuration <= KINDA_SMALL_NUMBER)
	{
		CancelFocusBlend();
		SetActorLocationAndRotation(DesiredLocation, DesiredRotation);
		return;
	}

	CancelFocusBlend();
	BlendStartLocation = GetActorLocation();
	BlendStartRotation = GetActorRotation().Quaternion();
	BlendTargetLocation = DesiredLocation;
	BlendTargetRotation = DesiredRotation;
	FocusBlendDuration = BlendDuration;
	FocusBlendElapsed = 0.f;
	bFocusBlending = true;
	SetActorTickEnabled(true);
}

void ADIVECameraRig::SyncOrbitFromCurrentView()
{
	const FVector CameraLocation = GetActorLocation();
	OrbitDistance = FMath::Clamp(FVector::Dist(CameraLocation, OrbitTarget), GetEffectiveMinOrbitDistance(), MaxOrbitDistance);

	const FVector LookDirection = (OrbitTarget - CameraLocation).GetSafeNormal();
	if (!LookDirection.IsNearlyZero())
	{
		OrbitOrientation = MakeLookOrientation(LookDirection);
		ClampOrientationPitch(OrbitOrientation);
	}
}

void ADIVECameraRig::SyncOrbitOrientationFromCurrentView()
{
	const FVector CameraLocation = GetActorLocation();
	const FVector LookDirection = (OrbitTarget - CameraLocation).GetSafeNormal();
	if (!LookDirection.IsNearlyZero())
	{
		OrbitOrientation = MakeLookOrientation(LookDirection);
		ClampOrientationPitch(OrbitOrientation);
	}
}

void ADIVECameraRig::ApplyFreeLookDelta(FQuat& Orientation, float YawRadians, float PitchRadians)
{
	if (!FMath::IsNearlyZero(YawRadians))
	{
		const FQuat YawQuat(FVector::UpVector, YawRadians);
		Orientation = (YawQuat * Orientation).GetNormalized();
	}

	if (!FMath::IsNearlyZero(PitchRadians))
	{
		const FVector RightAxis = Orientation.RotateVector(FVector::RightVector);
		const FQuat PitchQuat(RightAxis, PitchRadians);
		Orientation = (PitchQuat * Orientation).GetNormalized();
	}
}

void ADIVECameraRig::ClampOrientationPitch(FQuat& Orientation) const
{
	FRotator Rotation = Orientation.Rotator();
	Rotation.Pitch = FMath::Clamp(Rotation.Pitch, -MaxPitchDegrees, MaxPitchDegrees);
	Orientation = Rotation.Quaternion();
}

void ADIVECameraRig::ApplyOrbitDelta(const FVector2D& Delta)
{
	CancelFocusBlend();

	const float YawRadians = FMath::DegreesToRadians(Delta.X * OrbitInputScale);
	const float PitchRadians = FMath::DegreesToRadians(-Delta.Y * OrbitInputScale);

	if (Mode == EDIVECameraRigMode::AnchorViewpoint)
	{
		ApplyFreeLookDelta(AnchorViewOrientation, YawRadians, PitchRadians);
		ClampOrientationPitch(AnchorViewOrientation);
		RefreshCameraTransform();
		return;
	}

	ApplyFreeLookDelta(OrbitOrientation, YawRadians, PitchRadians);
	ClampOrientationPitch(OrbitOrientation);
	RefreshCameraTransform();
}

void ADIVECameraRig::ApplyZoomDelta(float Delta)
{
	CancelFocusBlend();

	if (Mode == EDIVECameraRigMode::AnchorViewpoint)
	{
		// Authored viewpoints use the base sensitivity; distance scaling is for orbit mode.
		AnchorViewLocation += AnchorViewOrientation.RotateVector(FVector::ForwardVector) * (Delta * ZoomInputScale);
		RefreshCameraTransform();
		return;
	}

	const float StepCm = ComputeZoomStepCm();
	OrbitDistance = FMath::Clamp(OrbitDistance - Delta * StepCm, GetEffectiveMinOrbitDistance(), MaxOrbitDistance);
	RefreshCameraTransform();
}

void ADIVECameraRig::ComputeDesiredTransform(FVector& OutLocation, FQuat& OutRotation) const
{
	if (Mode == EDIVECameraRigMode::AnchorViewpoint)
	{
		OutLocation = AnchorViewLocation;
		OutRotation = AnchorViewOrientation;
		return;
	}

	const FVector LookDirection = OrbitOrientation.RotateVector(FVector::ForwardVector);
	OutLocation = OrbitTarget - LookDirection * OrbitDistance;
	OutRotation = OrbitOrientation;
}

void ADIVECameraRig::RefreshCameraTransform()
{
	CancelFocusBlend();

	FVector DesiredLocation;
	FQuat DesiredRotation;
	ComputeDesiredTransform(DesiredLocation, DesiredRotation);
	SetActorLocationAndRotation(DesiredLocation, DesiredRotation);
}

void ADIVECameraRig::CancelFocusBlend()
{
	bFocusBlending = false;
	FocusBlendElapsed = 0.f;
	FocusBlendDuration = 0.f;
	SetActorTickEnabled(false);
}
