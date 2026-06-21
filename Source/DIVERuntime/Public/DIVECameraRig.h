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

	void SetInputSensitivity(float InOrbitSensitivity, float InZoomSensitivity);

	void SetOrbitTarget(const FVector& WorldLocation);
	void SetOrbitDistance(float Distance);
	void SetAnchorViewpoint(const FVector& WorldLocation, const FRotator& WorldRotation);
	void ApplyOrbitDelta(const FVector2D& Delta);
	void ApplyZoomDelta(float Delta);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "DIVE")
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(EditAnywhere, Category = "DIVE")
	float MinOrbitDistance = 50.f;

	UPROPERTY(EditAnywhere, Category = "DIVE")
	float MaxOrbitDistance = 2000.f;

private:
	EDIVECameraRigMode Mode = EDIVECameraRigMode::Orbit;

	float OrbitInputScale = 1.5f;
	float ZoomInputScale = 25.f;

	FVector OrbitTarget = FVector::ZeroVector;
	FQuat OrbitOrientation = FQuat::Identity;
	float OrbitDistance = 200.f;

	FVector AnchorViewLocation = FVector::ZeroVector;
	FQuat AnchorViewOrientation = FQuat::Identity;

	static void ApplyFreeLookDelta(FQuat& Orientation, float YawRadians, float PitchRadians);
	void SyncOrbitFromCurrentView();
	void RefreshCameraTransform();
};
