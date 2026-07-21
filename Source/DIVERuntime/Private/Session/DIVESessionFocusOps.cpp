// Copyright (c) 2026. All Rights Reserved.

#include "Session/DIVESessionFocusOps.h"

#include "DIVEAnchorComponent.h"
#include "DIVECameraRig.h"
#include "DIVEInspectableComponent.h"
#include "DIVELog.h"
#include "DIVESessionSubsystem.h"
#include "Session/DIVESessionIsolationOps.h"

bool FDIVESessionFocusOps::ApplyFocusTarget(
	UDIVESessionSubsystem& Session,
	const FDIVEFocusTarget& Target,
	const bool bPushToStack,
	const bool bBlendCamera,
	const bool bResetOrbitDistance)
{
	UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
	ADIVECameraRig* CameraRig = Session.ActiveCameraRig.Get();
	if (!Inspectable || !CameraRig)
	{
		return false;
	}

	if (bPushToStack)
	{
		if (Session.FocusStack.IsEmpty() || !Session.FocusStack.Last().Equals(Target))
		{
			Session.FocusStack.Add(Target);
		}
	}
	else if (Session.FocusStack.IsEmpty())
	{
		Session.FocusStack.Add(Target);
	}
	else
	{
		Session.FocusStack.Last() = Target;
	}

	Session.FocusedTarget = Target;

	constexpr bool bRefreshTransform = false;
	if (Session.FocusedTarget.Kind == EDIVEFocusKind::Anchor)
	{
		if (UDIVEAnchorComponent* Anchor = Cast<UDIVEAnchorComponent>(Session.FocusedTarget.Anchor.Get()))
		{
			CameraRig->SetAnchorViewpoint(Anchor->GetComponentLocation(), Anchor->GetViewRotation(), bRefreshTransform);
		}
		else
		{
			CameraRig->SetOrbitTarget(Session.FocusedTarget.GetPivotLocation(Session.ActiveDeviceHost.Get()), bRefreshTransform);
			CameraRig->SyncOrbitFromCurrentView();
		}
	}
	else
	{
		CameraRig->SetOrbitTarget(Session.FocusedTarget.GetPivotLocation(Session.ActiveDeviceHost.Get()), bRefreshTransform);
	}

	if (Session.FocusedTarget.Kind != EDIVEFocusKind::Anchor)
	{
		CameraRig->SetFocusClearanceRadius(Inspectable->ComputeFocusClearanceRadius(Session.FocusedTarget));

		if (bResetOrbitDistance)
		{
			const float OrbitDistance = Session.FocusedTarget.Kind == EDIVEFocusKind::Primitive
				? Inspectable->ComputeOrbitDistanceForFocus(Session.FocusedTarget)
				: Session.SessionDefaultOrbitDistance;
			CameraRig->SetOrbitDistance(OrbitDistance, bRefreshTransform);
			CameraRig->SyncOrbitOrientationFromCurrentView();
		}
		else
		{
			CameraRig->SyncOrbitFromCurrentView();
		}
	}
	else
	{
		CameraRig->SetFocusClearanceRadius(0.f);
	}

	const float BlendDuration = bBlendCamera ? Inspectable->GetEffectiveCameraSettings().FocusBlendDuration : 0.f;
	CameraRig->ApplyFocusPresentation(BlendDuration);

	if (Session.bIsolationActive)
	{
		FDIVESessionIsolationOps::ApplyIsolationForTarget(Session, Session.FocusedTarget);
	}

	Session.OnFocusChanged.Broadcast(Session.FocusedTarget);
	return true;
}

bool FDIVESessionFocusOps::ApplyInitialSessionFocus(UDIVESessionSubsystem& Session, const FName InitialFocusId)
{
	if (InitialFocusId.IsNone())
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
	AActor* DeviceHost = Session.ActiveDeviceHost.Get();
	if (!Inspectable || !DeviceHost)
	{
		return false;
	}

	FDIVEFocusTarget Target;
	if (!Inspectable->TryResolveStartFocusTarget(InitialFocusId, Target))
	{
		UE_LOG(
			LogDIVE,
			Warning,
			TEXT("DIVE: InitialFocusId '%s' not found on '%s'; falling back to device root."),
			*InitialFocusId.ToString(),
			*GetNameSafe(DeviceHost));
		return false;
	}

	Session.FocusStack.Reset();
	Session.FocusStack.Add(FDIVEFocusTarget::MakeDeviceRoot());
	const bool bResetOrbitDistance = Target.Kind == EDIVEFocusKind::Primitive;
	return ApplyFocusTarget(Session, Target, true, true, bResetOrbitDistance);
}
