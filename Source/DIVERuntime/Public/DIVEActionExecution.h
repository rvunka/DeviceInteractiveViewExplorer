// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEDeviceAction.h"

class UDIVEInspectableComponent;
class UWorld;

/**
 * Session-agnostic continuous-action slot. Owned by the session subsystem (monitor)
 * or by UDIVEInspectableComponent (headless / standing-VR host).
 */
struct DIVERUNTIME_API FDIVEContinuousActionSlot
{
	TWeakObjectPtr<UDIVEContinuousDeviceAction> ActiveAction;

	bool IsActive() const
	{
		const UDIVEContinuousDeviceAction* Action = ActiveAction.Get();
		return Action && Action->IsInteractionActive();
	}

	void Reset()
	{
		ActiveAction.Reset();
	}
};

/**
 * Catalog / Bindings execution without requiring an active camera session.
 * Session subsystem wrappers keep IsSessionActive guards; Inspectable exposes headless API.
 */
namespace DIVEActionExecution
{
	/** Instant Execute or continuous Begin. No IsSessionActive check. */
	DIVERUNTIME_API bool ExecuteResolvedAction(
		UWorld* World,
		UDIVEInspectableComponent* Inspectable,
		FDIVEContinuousActionSlot& Slot,
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context);

	DIVERUNTIME_API bool TryBeginContinuousAction(
		UWorld* World,
		UDIVEInspectableComponent* Inspectable,
		FDIVEContinuousActionSlot& Slot,
		UDIVEContinuousDeviceAction* Action,
		const FDIVEActionContext& Context);

	DIVERUNTIME_API void UpdateContinuousAction(
		UWorld* World,
		FDIVEContinuousActionSlot& Slot,
		const FDIVEInteractionUpdate& Update);

	DIVERUNTIME_API void EndContinuousAction(
		UWorld* World,
		UDIVEInspectableComponent* Inspectable,
		FDIVEContinuousActionSlot& Slot,
		bool bCommit);
}
