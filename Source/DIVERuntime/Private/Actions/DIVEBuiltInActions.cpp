// Copyright (c) 2026. All Rights Reserved.

#include "Actions/DIVEBuiltInActions.h"

#include "Components/PrimitiveComponent.h"
#include "DIVEProxyDrive.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVEProxyDriveTypes.h"
#include "DIVESessionSubsystem.h"
#include "Engine/World.h"

namespace
{
UDIVESessionSubsystem* ResolveSessionSubsystem(const UObject* WorldContext, const FDIVEActionContext& Context)
{
	// Try the device host first — it is always world-bound, even when the action itself is
	// instanced in a UDIVEActionCatalogAsset (where WorldContext->GetWorld() returns nullptr).
	const AActor* Host = Context.DeviceHost.Get();
	UWorld* World = Host ? Host->GetWorld() : nullptr;
	if (!World)
	{
		World = WorldContext ? WorldContext->GetWorld() : nullptr;
	}
	return World ? World->GetSubsystem<UDIVESessionSubsystem>() : nullptr;
}
} // namespace

UDIVEFocusAction::UDIVEFocusAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuFocus", "Focus");
}

bool UDIVEFocusAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	return Context.PickTarget.Kind == EDIVEFocusKind::Primitive && Context.PickTarget.IsValidFocus();
}

bool UDIVEFocusAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	const bool bHandled = Subsystem->FocusTarget(Context.PickTarget, true);
	if (bHandled)
	{
		OnExecuted.Broadcast(this, Context);
	}
	return bHandled;
}

UDIVEIsolateAction::UDIVEIsolateAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuIsolate", "Isolate");
}

bool UDIVEIsolateAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	return Subsystem
		&& Subsystem->IsSessionActive()
		&& Context.PickTarget.Kind == EDIVEFocusKind::Primitive
		&& Context.PickTarget.IsValidFocus();
}

FDIVEActionDisplayState UDIVEIsolateAction::GetDisplayState_Implementation(const FDIVEActionContext& Context) const
{
	FDIVEActionDisplayState State = Super::GetDisplayState_Implementation(Context);
	if (const UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context))
	{
		State.bChecked = Subsystem->IsIsolationActiveForTarget(Context.PickTarget);
	}
	return State;
}

bool UDIVEIsolateAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	const bool bHandled = Subsystem->ToggleIsolationForTarget(Context.PickTarget);
	if (bHandled)
	{
		OnExecuted.Broadcast(this, Context);
	}
	return bHandled;
}

UDIVESimulatePhysicsAction::UDIVESimulatePhysicsAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuSimulatePhysics", "Simulate Physics");
}

bool UDIVESimulatePhysicsAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	return UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed()
		&& Context.PickTarget.Kind == EDIVEFocusKind::Primitive
		&& Context.Target != nullptr;
}

FDIVEActionDisplayState UDIVESimulatePhysicsAction::GetDisplayState_Implementation(const FDIVEActionContext& Context) const
{
	FDIVEActionDisplayState State = Super::GetDisplayState_Implementation(Context);
	State.bVisible = UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed();
	State.bChecked = Context.Target && Context.Target->IsSimulatingPhysics();
	return State;
}

bool UDIVESimulatePhysicsAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	const bool bHandled = Subsystem->ToggleMeshPhysicsForTarget(Context.PickTarget);
	if (bHandled)
	{
		OnExecuted.Broadcast(this, Context);
	}
	return bHandled;
}

UDIVEDeleteMeshAction::UDIVEDeleteMeshAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuDeleteMesh", "Delete Mesh");
}

bool UDIVEDeleteMeshAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context))
	{
		return false;
	}

	return UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed()
		&& Context.PickTarget.Kind == EDIVEFocusKind::Primitive
		&& Context.Target != nullptr;
}

FDIVEActionDisplayState UDIVEDeleteMeshAction::GetDisplayState_Implementation(const FDIVEActionContext& Context) const
{
	FDIVEActionDisplayState State = Super::GetDisplayState_Implementation(Context);
	State.bVisible = UDIVESessionSubsystem::AreAdminContextMenuEntriesAllowed();
	return State;
}

bool UDIVEDeleteMeshAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	UDIVESessionSubsystem* Subsystem = ResolveSessionSubsystem(this, Context);
	if (!Subsystem || !CanExecute(Context))
	{
		return false;
	}

	const bool bHandled = Subsystem->DeleteMeshForTarget(Context.PickTarget);
	if (bHandled)
	{
		OnExecuted.Broadcast(this, Context);
	}
	return bHandled;
}

UDIVEProxyDriveForwardAction::UDIVEProxyDriveForwardAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ProxyDriveForward", "Drive");
}

bool UDIVEProxyDriveForwardAction::CanExecute_Implementation(const FDIVEActionContext& Context) const
{
	if (!Super::CanExecute_Implementation(Context) || !Context.Target)
	{
		return false;
	}

	if (IDIVEProxyDrive* ProxyDrive = DIVEProxyDriveResolve::FindProxyDriveForHit(Context.Target))
	{
		if (UObject* ProxyObject = Cast<UObject>(ProxyDrive))
		{
			return IDIVEProxyDrive::Execute_CanProxyDrive(ProxyObject);
		}
	}
	return false;
}

bool UDIVEProxyDriveForwardAction::BeginInteraction_Implementation(const FDIVEActionContext& Context)
{
	ActiveProxyObject.Reset();
	if (!Context.Target)
	{
		return false;
	}

	IDIVEProxyDrive* ProxyDrive = DIVEProxyDriveResolve::FindProxyDriveForHit(Context.Target);
	UObject* ProxyObject = Cast<UObject>(ProxyDrive);
	if (!ProxyObject || !IDIVEProxyDrive::Execute_CanProxyDrive(ProxyObject))
	{
		return false;
	}

	FDIVEProxyDriveContext DriveContext;
	DriveContext.ScreenPosition = Context.ScreenPosition;
	DriveContext.FocusTarget = Context.PickTarget;
	DriveContext.HitComponent = Context.Target;
	DriveContext.PickHit = Context.PickHit;

	if (!IDIVEProxyDrive::Execute_BeginProxyDrive(ProxyObject, DriveContext))
	{
		return false;
	}

	ActiveProxyObject = ProxyObject;
	return true;
}

void UDIVEProxyDriveForwardAction::UpdateInteraction_Implementation(FVector2D ScreenDelta, float DeltaTime)
{
	(void)DeltaTime;
	if (UObject* ProxyObject = ActiveProxyObject.Get())
	{
		IDIVEProxyDrive::Execute_ApplyProxyDriveDelta(ProxyObject, ScreenDelta);
	}
}

void UDIVEProxyDriveForwardAction::EndInteraction_Implementation(bool bCommit)
{
	if (UObject* ProxyObject = ActiveProxyObject.Get())
	{
		IDIVEProxyDrive::Execute_EndProxyDrive(ProxyObject, bCommit);
	}
	ActiveProxyObject.Reset();
	NotifyInteractionCompleted();
}

UDIVENotifyAction::UDIVENotifyAction()
{
	DisplayName = NSLOCTEXT("DIVE", "ContextMenuNotify", "Notify");
}

bool UDIVENotifyAction::Execute_Implementation(const FDIVEActionContext& Context)
{
	if (!CanExecute(Context))
	{
		return false;
	}

	OnExecuted.Broadcast(this, Context);
	return true;
}
