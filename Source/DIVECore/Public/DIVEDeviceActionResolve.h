// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceActionDefinition.h"
#include "DIVETypes.h"

namespace DIVEDeviceActionResolve
{
inline FName ResolveActionId(const FDIVEPickContextMenuAction& Action)
{
	return Action.Definition ? Action.Definition->ActionId : NAME_None;
}

inline FText ResolveDisplayName(const FDIVEPickContextMenuAction& Action)
{
	if (!Action.DisplayName.IsEmpty())
	{
		return Action.DisplayName;
	}
	if (Action.Definition && !Action.Definition->DefaultDisplayName.IsEmpty())
	{
		return Action.Definition->DefaultDisplayName;
	}
	const FName Id = ResolveActionId(Action);
	return Id.IsNone() ? FText::GetEmpty() : FText::FromName(Id);
}

inline bool ResolveToggleActiveSuffix(const FDIVEPickContextMenuAction& Action)
{
	return Action.Definition && Action.Definition->bToggleActiveSuffix;
}
}
