// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceDefinitionAsset.h"

#if WITH_EDITOR

#include "Misc/DataValidation.h"

EDataValidationResult UDIVEDeviceDefinitionAsset::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (DefaultOrbitDistance <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("DefaultOrbitDistance must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (OrbitSensitivity <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("OrbitSensitivity must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (ZoomSensitivity <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("ZoomSensitivity must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (MinOrbitDistanceCm <= 0.f || MaxOrbitDistanceCm < MinOrbitDistanceCm)
	{
		Context.AddError(FText::FromString(TEXT("Orbit distance limits must satisfy 0 < Min <= Max.")));
		Result = EDataValidationResult::Invalid;
	}

	if (bScaleZoomWithOrbitDistance && ZoomDistanceReferenceCm <= 0.f)
	{
		Context.AddError(FText::FromString(TEXT("ZoomDistanceReferenceCm must be greater than zero.")));
		Result = EDataValidationResult::Invalid;
	}

	if (FocusBlendDuration < 0.f)
	{
		Context.AddError(FText::FromString(TEXT("FocusBlendDuration must be greater than or equal to zero.")));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

#endif // WITH_EDITOR
