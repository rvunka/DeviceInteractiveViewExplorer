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
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(RootComponent);

	OrbitOrientation = FRotator(-20.f, 0.f, 0.f).Quaternion();
}

void ADIVECameraRig::SetInputSensitivity(float InOrbitSensitivity, float InZoomSensitivity)
{
	OrbitInputScale = FMath::Max(InOrbitSensitivity, 0.01f);
	ZoomInputScale = FMath::Max(InZoomSensitivity, 0.01f);
}

void ADIVECameraRig::SetOrbitTarget(const FVector& WorldLocation)
{
	const bool bLeaveAnchorViewpoint = Mode == EDIVECameraRigMode::AnchorViewpoint;
	Mode = EDIVECameraRigMode::Orbit;
	OrbitTarget = WorldLocation;

	if (bLeaveAnchorViewpoint)
	{
		SyncOrbitFromCurrentView();
	}

	RefreshCameraTransform();
}

void ADIVECameraRig::SetOrbitDistance(float Distance)
{
	OrbitDistance = FMath::Clamp(Distance, MinOrbitDistance, MaxOrbitDistance);
	if (Mode == EDIVECameraRigMode::Orbit)
	{
		RefreshCameraTransform();
	}
}

void ADIVECameraRig::SetAnchorViewpoint(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	Mode = EDIVECameraRigMode::AnchorViewpoint;
	AnchorViewLocation = WorldLocation;
	AnchorViewOrientation = WorldRotation.Quaternion();
	RefreshCameraTransform();
}

void ADIVECameraRig::SyncOrbitFromCurrentView()
{
	const FVector CameraLocation = GetActorLocation();
	OrbitDistance = FMath::Clamp(FVector::Dist(CameraLocation, OrbitTarget), MinOrbitDistance, MaxOrbitDistance);

	const FVector LookDirection = (OrbitTarget - CameraLocation).GetSafeNormal();
	if (!LookDirection.IsNearlyZero())
	{
		OrbitOrientation = MakeLookOrientation(LookDirection);
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

void ADIVECameraRig::ApplyOrbitDelta(const FVector2D& Delta)
{
	const float YawRadians = FMath::DegreesToRadians(Delta.X * OrbitInputScale);
	const float PitchRadians = FMath::DegreesToRadians(-Delta.Y * OrbitInputScale);

	if (Mode == EDIVECameraRigMode::AnchorViewpoint)
	{
		ApplyFreeLookDelta(AnchorViewOrientation, YawRadians, PitchRadians);
		RefreshCameraTransform();
		return;
	}

	ApplyFreeLookDelta(OrbitOrientation, YawRadians, PitchRadians);
	RefreshCameraTransform();
}

void ADIVECameraRig::ApplyZoomDelta(float Delta)
{
	if (Mode == EDIVECameraRigMode::AnchorViewpoint)
	{
		AnchorViewLocation += AnchorViewOrientation.RotateVector(FVector::ForwardVector) * (Delta * ZoomInputScale);
		RefreshCameraTransform();
		return;
	}

	OrbitDistance = FMath::Clamp(OrbitDistance - Delta * ZoomInputScale, MinOrbitDistance, MaxOrbitDistance);
	RefreshCameraTransform();
}

void ADIVECameraRig::RefreshCameraTransform()
{
	if (Mode == EDIVECameraRigMode::AnchorViewpoint)
	{
		SetActorLocation(AnchorViewLocation);
		SetActorRotation(AnchorViewOrientation);
		return;
	}

	const FVector LookDirection = OrbitOrientation.RotateVector(FVector::ForwardVector);
	const FVector CameraLocation = OrbitTarget - LookDirection * OrbitDistance;
	SetActorLocation(CameraLocation);
	SetActorRotation(OrbitOrientation);
}
