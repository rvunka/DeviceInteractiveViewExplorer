// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceActionDefinition.h"

#include "DIVEConvention.h"

#if WITH_EDITOR

EDataValidationResult UDIVEDeviceActionDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (ActionId.IsNone())
	{
		// Warning only: Error blocks Content Browser rename/save before authors finish editing.
		// Catalog Validate/Scan still treat empty resolved ActionId as an error when the asset is used.
		Context.AddWarning(FText::FromString(TEXT("ActionId is empty — set it before assigning this asset to a catalog row.")));
	}
	else if (DIVE::IsReservedContextMenuActionId(ActionId))
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("ActionId '%s' is reserved by DIVE built-in menu rows."),
			*ActionId.ToString())));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

#endif // WITH_EDITOR
