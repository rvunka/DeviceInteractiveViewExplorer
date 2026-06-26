// Copyright (c) 2026. All Rights Reserved.

#include "Utils/DIVEOperations.h"

#include "DIVEDeviceDefinitionAsset.h"

namespace DIVEOperations
{
bool FindCatalogDescriptor(
	const UDIVEDeviceDefinitionAsset* DeviceDefinition,
	FName OperationId,
	FDIVEOperationDescriptor& OutDescriptor)
{
	if (!DeviceDefinition || OperationId.IsNone())
	{
		return false;
	}

	for (const FDIVEOperationDescriptor& Descriptor : DeviceDefinition->OperationCatalog)
	{
		if (Descriptor.OperationId == OperationId)
		{
			OutDescriptor = Descriptor;
			return true;
		}
	}

	return false;
}

FDIVEOperationDescriptor MakeFallbackDescriptor(FName OperationId)
{
	FDIVEOperationDescriptor Descriptor;
	Descriptor.OperationId = OperationId;
	Descriptor.DisplayName = FText::FromName(OperationId);
	Descriptor.InputMode = EDIVEOperationInputMode::Press;
	Descriptor.HoldDuration = 0.45f;
	return Descriptor;
}

bool ValidateOperation(
	const UDIVEDeviceDefinitionAsset* DeviceDefinition,
	FName OperationId,
	const TSet<FName>& CompletedOperationIds,
	FText& OutFailureMessage)
{
	if (OperationId.IsNone())
	{
		OutFailureMessage = NSLOCTEXT("DIVE", "MissingOperationId", "Operation id is not set.");
		return false;
	}

	if (!DeviceDefinition)
	{
		return true;
	}

	for (const FDIVEOperationValidationRule& Rule : DeviceDefinition->ValidationRules)
	{
		if (Rule.OperationId != OperationId)
		{
			continue;
		}

		for (const FName RequiredOperationId : Rule.RequiredCompletedOperationIds)
		{
			if (!CompletedOperationIds.Contains(RequiredOperationId))
			{
				OutFailureMessage = Rule.FailureMessage.IsEmpty()
					? FText::Format(
						NSLOCTEXT("DIVE", "MissingPrerequisite", "Complete '{0}' before this operation."),
						FText::FromName(RequiredOperationId))
					: Rule.FailureMessage;
				return false;
			}
		}
	}

	return true;
}
}
