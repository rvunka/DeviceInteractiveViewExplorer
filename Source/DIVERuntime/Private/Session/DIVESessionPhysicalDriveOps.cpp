// Copyright (c) 2026. All Rights Reserved.

#include "Session/DIVESessionPhysicalDriveOps.h"

#include "Actions/DIVEBuiltInActions.h"
#include "DIVEDeviceAction.h"
#include "DIVELog.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVEInspectableComponent.h"
#include "DIVECameraRig.h"
#include "DIVESessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
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
		if (UDIVEContinuousDeviceAction* Continuous = Session.ActiveContinuousAction.Get())
		{
			FDIVEActionWorldScope WorldScope(Continuous, Session.GetWorld());
			Continuous->OnValueChanged.RemoveDynamic(
				&Session,
				&UDIVESessionSubsystem::HandleContinuousActionValueChanged);
			Continuous->EndInteraction(bCommit);
		}
		break;
	default:
		break;
	}
}

void FDIVESessionPhysicalDriveOps::ResetPhysicalDriveState(UDIVESessionSubsystem& Session)
{
	Session.bProxyDriving = false;
	Session.ActivePhysicalDriveKind = UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::None;
	Session.ActivePawnPhysicalDrive.Reset();
	Session.ActiveContinuousAction.Reset();
	Session.bIgnoreNextPrimaryActionRelease = false;
}

void FDIVESessionPhysicalDriveOps::ClearProxyDrive(UDIVESessionSubsystem& Session)
{
	if (!Session.bProxyDriving)
	{
		return;
	}

	EndActivePhysicalDrive(Session, false);
	ResetPhysicalDriveState(Session);
	Session.NotifyInteractionValueChanged(nullptr, FDIVEActionContext(), FDIVEInteractionValue());
}

bool FDIVESessionPhysicalDriveOps::TryBeginProxyDriveAtScreenPosition(
	UDIVESessionSubsystem& Session,
	const FVector2D& ScreenPosition,
	APlayerController* PlayerController)
{
	if (!Session.IsSessionActive() || Session.bProxyDriving || !PlayerController)
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

	// Device proxy path: route through InternalProxyDriveAction so the interaction uses the single
	// continuous-action slot instead of a parallel device-proxy code path.
	if (DIVEProxyDriveResolve::FindProxyDriveForHit(HitComponent))
	{
		UDIVEInspectableComponent* Inspectable = Session.ActiveInspectable.Get();
		if (Inspectable && Session.InternalProxyDriveAction)
		{
			const FDIVEActionContext Context = Inspectable->MakeActionContext(
				PickTarget,
				NAME_None,
				ScreenPosition,
				HitResult);
			if (Session.TryBeginContinuousAction(Session.InternalProxyDriveAction.Get(), Context))
			{
				return true;
			}
		}
	}

	// Pawn physical drive (e.g. GRIP): own lifecycle, separate from the action system.
	FDIVEProxyDriveContext DriveContext;
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
				Session.bProxyDriving = true;
				return true;
			}
		}
	}

	UE_LOG(
		LogDIVE,
		Warning,
		TEXT("DIVE: Physical pick on '%s' had no device proxy drive and no pawn physical drive backend."),
		*GetNameSafe(HitComponent));

	return false;
}

void FDIVESessionPhysicalDriveOps::UpdateActiveInteraction(UDIVESessionSubsystem& Session, const FVector2D& ScreenDelta)
{
	if (!Session.bProxyDriving)
	{
		return;
	}

	switch (Session.ActivePhysicalDriveKind)
	{
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive:
		if (!Session.ActivePawnPhysicalDrive.GetObject())
		{
			ClearProxyDrive(Session);
		}
		break;
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::ContinuousAction:
		if (UDIVEContinuousDeviceAction* Continuous = Session.ActiveContinuousAction.Get())
		{
			const float DeltaTime = Session.GetWorld() ? Session.GetWorld()->GetDeltaSeconds() : 0.f;
			{
				FDIVEActionWorldScope WorldScope(Continuous, Session.GetWorld());
				Continuous->UpdateInteraction(ScreenDelta, DeltaTime);
			}
			if (!Continuous->IsInteractionActive())
			{
				EndProxyDrive(Session, true);
			}
		}
		else
		{
			ClearProxyDrive(Session);
		}
		break;
	default:
		ClearProxyDrive(Session);
		break;
	}
}

void FDIVESessionPhysicalDriveOps::EndProxyDrive(UDIVESessionSubsystem& Session, const bool bCommit)
{
	if (!Session.bProxyDriving)
	{
		return;
	}

	EndActivePhysicalDrive(Session, bCommit);
	ResetPhysicalDriveState(Session);
	Session.NotifyInteractionValueChanged(nullptr, FDIVEActionContext(), FDIVEInteractionValue());
}

void FDIVESessionPhysicalDriveOps::HandleActivePawnPhysicalManualRotatePressed(UDIVESessionSubsystem& Session)
{
	if (!Session.bProxyDriving || Session.ActivePhysicalDriveKind != UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive)
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
	if (!Session.bProxyDriving || Session.ActivePhysicalDriveKind != UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnDrive)
	{
		return;
	}

	if (UObject* PawnDriveObject = Session.ActivePawnPhysicalDrive.GetObject())
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotateReleased(PawnDriveObject);
	}
}
