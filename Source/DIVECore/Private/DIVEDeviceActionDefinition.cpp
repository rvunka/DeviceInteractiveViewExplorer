// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceActionDefinition.h"

#include "DIVEConvention.h"

#if WITH_EDITOR

EDataValidationResult UDIVEDeviceActionDefinition::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	if (ActionId.IsNone())
	{
		Context.AddError(FText::FromString(TEXT("ActionId must be set.")));
		Result = EDataValidationResult::Invalid;
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
