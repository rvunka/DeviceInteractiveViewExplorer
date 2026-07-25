// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEDeviceActionDefinition.h"
#include "DIVETypes.h"
#include "DIVEUnscrewActionDefinition.h"

namespace DIVEDeviceActionResolve
{
inline FName ResolveActionId(const FDIVEPickContextMenuAction& Action)
{
	if (Action.Definition)
	{
		return Action.Definition->ActionId;
	}
	return Action.ActionId;
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
	if (Action.Definition)
	{
		return Action.Definition->bToggleActiveSuffix;
	}
	return Action.bToggleActiveSuffix;
}

inline int32 ResolveUnscrewTurnCount(const FDIVEPickContextMenuAction& Action)
{
	if (Action.InstanceOverrides.UnscrewTurnCount != INDEX_NONE)
	{
		return Action.InstanceOverrides.UnscrewTurnCount;
	}
	if (const UDIVEUnscrewActionDefinition* Unscrew = Cast<UDIVEUnscrewActionDefinition>(Action.Definition.Get()))
	{
		return Unscrew->DefaultTurnCount;
	}
	return INDEX_NONE;
}
}
