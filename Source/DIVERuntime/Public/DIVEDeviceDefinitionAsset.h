// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEConvention.h"
#include "Engine/DataAsset.h"
#include "DIVEDeviceDefinitionAsset.generated.h"

UCLASS(BlueprintType)
class DIVERUNTIME_API UDIVEDeviceDefinitionAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|View")
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "0.01"))
	float OrbitSensitivity = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Camera", meta = (ClampMin = "0.01"))
	float ZoomSensitivity = 40.f;
};
