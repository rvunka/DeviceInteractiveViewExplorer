// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceDefinitionAsset.h"

#if WITH_EDITOR

#include "Misc/DataValidation.h"

namespace
{
EDataValidationResult ValidateCameraSettings(
	const FDIVECameraSettings& Settings,
	FDataValidationContext& Context)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (Settings.DefaultOrbitDistance <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("DefaultOrbitDistance must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (Settings.OrbitSensitivity <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("OrbitSensitivity must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (Settings.ZoomSensitivity <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("ZoomSensitivity must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (Settings.MinOrbitDistanceCm <= 0.f || Settings.MaxOrbitDistanceCm < Settings.MinOrbitDistanceCm)
	{
		Context.AddError(FText::FromString(TEXT("Orbit distance limits must satisfy 0 < Min <= Max.")));
		Result = EDataValidationResult::Invalid;
	}

	if (Settings.bScaleZoomWithOrbitDistance && Settings.ZoomDistanceReferenceCm <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("ZoomDistanceReferenceCm must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (Settings.FocusBlendDuration < 0.f)
	{
		Context.AddError(FText::FromString(TEXT("FocusBlendDuration must be greater than or equal to zero.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
}

EDataValidationResult UDIVEDeviceDefinitionAsset::IsDataValid(FDataValidationContext& Context) const
{
	return ValidateCameraSettings(CameraSettings, Context);
}

#endif // WITH_EDITOR
