// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEConvention.h"
#include "Engine/DataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "DIVEDeviceDefinitionAsset.generated.h"

UCLASS(BlueprintType)
class DIVERUNTIME_API UDIVEDeviceDefinitionAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|View", meta = (ClampMin = "1.0"))
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "0.01"))
	float OrbitSensitivity = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "0.01"))
	float ZoomSensitivity = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera")
	bool bScaleZoomWithOrbitDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (
		ClampMin = "1.0",
		EditCondition = "bScaleZoomWithOrbitDistance"))
	float ZoomDistanceReferenceCm = DIVE::kDefaultZoomDistanceReference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "1.0"))
	float MinOrbitDistanceCm = DIVE::kDefaultMinOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "1.0"))
	float MaxOrbitDistanceCm = DIVE::kDefaultMaxOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "1.0"))
	float FocusOrbitFitMultiplier = DIVE::kDefaultFocusOrbitFitMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "0.0"))
	float FocusNearPaddingFactor = DIVE::kDefaultFocusNearPaddingFactor;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
