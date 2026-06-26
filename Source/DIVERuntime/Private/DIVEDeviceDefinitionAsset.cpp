// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceDefinitionAsset.h"

#if WITH_EDITOR

#include "Misc/DataValidation.h"

namespace
{
bool HasDuplicateOperationIds(const TArray<FDIVEOperationDescriptor>& Catalog)
{
	TSet<FName> SeenIds;
	for (const FDIVEOperationDescriptor& Descriptor : Catalog)
	{
		if (Descriptor.OperationId.IsNone())
		{
			continue;
		}

		if (SeenIds.Contains(Descriptor.OperationId))
		{
			return true;
		}

		SeenIds.Add(Descriptor.OperationId);
	}

	return false;
}
}

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

	if (HasDuplicateOperationIds(OperationCatalog))
	{
		Context.AddError(FText::FromString(TEXT("OperationCatalog contains duplicate OperationId values.")));
		Result = EDataValidationResult::Invalid;
	}

	for (const FDIVEOperationValidationRule& Rule : ValidationRules)
	{
		if (Rule.OperationId.IsNone())
		{
			Context.AddWarning(FText::FromString(TEXT("Validation rule with empty OperationId will never match.")));
		}
	}

	return Result;
}

#endif // WITH_EDITOR
