// Copyright (c) 2026. All Rights Reserved.

#include "Session/DIVESessionPhysicalDriveOps.h"

#include "DIVEActionExecution.h"
#include "DIVEDeviceAction.h"
#include "DIVELog.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVECameraRig.h"
#include "DIVESessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Session/DIVESessionPickOps.h"

void FDIVESessionPhysicalDriveOps::EndActivePhysicalDrive(UDIVESessionSubsystem& Session, const bool bCommit)
{
	switch (Session.ActivePhysicalDriveKind)
	{
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive:
		if (UObject* PawnDriveObject = Session.ActivePawnPhysicalDrive.GetObject())
		{
			IDIVEPawnPhysicalDrive::Execute_EndPawnPhysicalDrive(PawnDriveObject, bCommit);
		}
		break;
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::ContinuousAction:
	{
		UDIVEContinuousDeviceAction* Continuous = Session.ContinuousSlot.ActiveAction.Get();
		DIVEActionExecution::EndContinuousAction(
			Session.GetWorld(),
			Session.ActiveInspectable.Get(),
			Session.ContinuousSlot,
			bCommit);
		if (Continuous)
		{
			Continuous->OnValueChanged.RemoveDynamic(
				&Session,
				&UDIVESessionSubsystem::HandleContinuousActionValueChanged);
		}
		break;
	}
	default:
		break;
	}
}

void FDIVESessionPhysicalDriveOps::ResetPhysicalDriveState(UDIVESessionSubsystem& Session)
{
	Session.ActivePhysicalDriveKind = UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::None;
	Session.ActivePawnPhysicalDrive.Reset();
	Session.ContinuousSlot.Reset();
	Session.bIgnoreNextPrimaryActionRelease = false;
}

bool FDIVESessionPhysicalDriveOps::TryBeginPawnGrabAtScreenPosition(
	UDIVESessionSubsystem& Session,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	if (!Session.IsSessionActive() || Session.IsSessionGestureActive() || !PlayerController)
	{
		return false;
	}

	if (Session.InteractionMode != EDIVESessionInteractionMode::Physical)
	{
		return false;
	}

	FHitResult HitResult;
	FDIVEFocusTarget PickTarget;
	if (!FDIVESessionPickOps::ResolvePickAtScreenPositionWithHit(Session, ScreenPosition, PlayerController, PickTarget, HitResult))
	{
		return false;
	}

	UPrimitiveComponent* HitComponent = HitResult.GetComponent();
	if (!HitComponent)
	{
		return false;
	}

	FDIVEPawnPhysicalDriveContext DriveContext;
	DriveContext.ScreenPosition = ScreenPosition;
	DriveContext.FocusTarget = PickTarget;
	DriveContext.HitComponent = HitComponent;
	DriveContext.PickHit = HitResult;
	if (const ADIVECameraRig* CameraRig = Session.ActiveCameraRig.Get())
	{
		DriveContext.ViewLocation = CameraRig->GetActorLocation();
		DriveContext.ViewRotation = CameraRig->GetActorRotation();
	}
	if (HitResult.bBlockingHit || !HitResult.TraceStart.Equals(HitResult.TraceEnd))
	{
		DriveContext.PickRayDir = (HitResult.TraceEnd - HitResult.TraceStart).GetSafeNormal();
	}
	if (DriveContext.PickRayDir.IsNearlyZero())
	{
		DriveContext.PickRayDir = DriveContext.ViewRotation.Vector();
	}

	if (IDIVEPawnPhysicalDrive* PawnDrive = DIVEPawnPhysicalDriveResolve::FindOnPlayerController(PlayerController))
	{
		if (UObject* PawnDriveObject = Cast<UObject>(PawnDrive))
		{
			if (IDIVEPawnPhysicalDrive::Execute_CanBeginPawnPhysicalDrive(PawnDriveObject, DriveContext)
				&& IDIVEPawnPhysicalDrive::Execute_BeginPawnPhysicalDrive(PawnDriveObject, DriveContext))
			{
				Session.ActivePhysicalDriveKind = UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive;
				Session.ActivePawnPhysicalDrive = PawnDriveObject;
				return true;
			}
		}
	}

	UE_LOG(
		LogDIVE,
		Warning,
		TEXT("DIVE: Physical pick on '%s' had no pawn physical drive backend."),
		*GetNameSafe(HitComponent));

	return false;
}

void FDIVESessionPhysicalDriveOps::UpdateActiveInteraction(UDIVESessionSubsystem& Session, const FDIVEInteractionUpdate& Update)
{
	if (!Session.IsSessionGestureActive())
	{
		return;
	}

	switch (Session.ActivePhysicalDriveKind)
	{
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive:
		if (!Session.ActivePawnPhysicalDrive.GetObject())
		{
			EndSessionGesture(Session, false);
		}
		break;
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::ContinuousAction:
		if (UDIVEContinuousDeviceAction* Continuous = Session.ContinuousSlot.ActiveAction.Get())
		{
			DIVEActionExecution::UpdateContinuousAction(Session.GetWorld(), Session.ContinuousSlot, Update);
			if (!Continuous->IsInteractionActive())
			{
				EndSessionGesture(Session, true);
			}
		}
		else
		{
			EndSessionGesture(Session, false);
		}
		break;
	default:
		EndSessionGesture(Session, false);
		break;
	}
}

void FDIVESessionPhysicalDriveOps::EndSessionGesture(UDIVESessionSubsystem& Session, const bool bCommit)
{
	if (!Session.IsSessionGestureActive())
	{
		return;
	}

	EndActivePhysicalDrive(Session, bCommit);
	ResetPhysicalDriveState(Session);
	Session.NotifyInteractionValueChanged(nullptr, FDIVEActionContext(), FDIVEInteractionValue());
}

void FDIVESessionPhysicalDriveOps::HandleActivePawnPhysicalManualRotatePressed(UDIVESessionSubsystem& Session)
{
	if (!Session.IsSessionGestureActive() || Session.ActivePhysicalDriveKind != UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive)
	{
		return;
	}

	if (UObject* PawnDriveObject = Session.ActivePawnPhysicalDrive.GetObject())
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotatePressed(PawnDriveObject);
	}
}

void FDIVESessionPhysicalDriveOps::HandleActivePawnPhysicalManualRotateReleased(UDIVESessionSubsystem& Session)
{
	if (!Session.IsSessionGestureActive() || Session.ActivePhysicalDriveKind != UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive)
	{
		return;
	}

	if (UObject* PawnDriveObject = Session.ActivePawnPhysicalDrive.GetObject())
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotateReleased(PawnDriveObject);
	}
}
