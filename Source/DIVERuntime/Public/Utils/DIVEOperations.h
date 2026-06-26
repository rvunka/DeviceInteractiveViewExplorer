// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVETypes.h"

class UDIVEDeviceDefinitionAsset;

namespace DIVEOperations
{
DIVERUNTIME_API bool FindCatalogDescriptor(
	const UDIVEDeviceDefinitionAsset* DeviceDefinition,
	FName OperationId,
	FDIVEOperationDescriptor& OutDescriptor);

DIVERUNTIME_API FDIVEOperationDescriptor MakeFallbackDescriptor(FName OperationId);

DIVERUNTIME_API bool ValidateOperation(
	const UDIVEDeviceDefinitionAsset* DeviceDefinition,
	FName OperationId,
	const TSet<FName>& CompletedOperationIds,
	FText& OutFailureMessage);
}
