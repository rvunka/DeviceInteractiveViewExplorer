// Copyright (c) 2026. All Rights Reserved.

#include "Session/DIVESessionPhysicalDriveOps.h"

#include "DIVELog.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVEProxyDrive.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVEProxyDriveTypes.h"
#include "DIVESessionSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Session/DIVESessionPickOps.h"

void FDIVESessionPhysicalDriveOps::EndActivePhysicalDrive(UDIVESessionSubsystem& Session, const bool bCommit)
{
	switch (Session.ActivePhysicalDriveKind)
	{
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::DeviceProxy:
		if (UObject* ProxyObject = Session.ActiveProxyDrive.GetObject())
		{
			IDIVEProxyDrive::Execute_EndProxyDrive(ProxyObject, bCommit);
		}
		break;
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnBridge:
		if (UObject* PawnDriveObject = Session.ActivePawnPhysicalDrive.GetObject())
		{
			IDIVEPawnPhysicalDrive::Execute_EndPawnPhysicalDrive(PawnDriveObject, bCommit);
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
	Session.ActiveProxyDrive.Reset();
	Session.ActivePawnPhysicalDrive.Reset();
}

void FDIVESessionPhysicalDriveOps::ClearProxyDrive(UDIVESessionSubsystem& Session)
{
	if (!Session.bProxyDriving)
	{
		return;
	}

	EndActivePhysicalDrive(Session, false);
	ResetPhysicalDriveState(Session);
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

	FDIVEProxyDriveContext DriveContext;
	DriveContext.ScreenPosition = ScreenPosition;
	DriveContext.FocusTarget = PickTarget;
	DriveContext.HitComponent = HitComponent;
	DriveContext.PickHit = HitResult;

	if (IDIVEProxyDrive* ProxyDrive = DIVEProxyDriveResolve::FindProxyDriveForHit(HitComponent))
	{
		if (UObject* ProxyObject = Cast<UObject>(ProxyDrive))
		{
			if (IDIVEProxyDrive::Execute_CanProxyDrive(ProxyObject)
				&& IDIVEProxyDrive::Execute_BeginProxyDrive(ProxyObject, DriveContext))
			{
				Session.ActivePhysicalDriveKind = UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::DeviceProxy;
				Session.ActiveProxyDrive = ProxyObject;
				Session.ActivePawnPhysicalDrive.Reset();
				Session.bProxyDriving = true;
				return true;
			}
		}
	}

	if (IDIVEPawnPhysicalDrive* PawnDrive = DIVEPawnPhysicalDriveResolve::FindOnPlayerController(PlayerController))
	{
		if (UObject* PawnDriveObject = Cast<UObject>(PawnDrive))
		{
			if (IDIVEPawnPhysicalDrive::Execute_CanBeginPawnPhysicalDrive(PawnDriveObject, DriveContext)
				&& IDIVEPawnPhysicalDrive::Execute_BeginPawnPhysicalDrive(PawnDriveObject, DriveContext))
			{
				Session.ActivePhysicalDriveKind = UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnBridge;
				Session.ActivePawnPhysicalDrive = PawnDriveObject;
				Session.ActiveProxyDrive.Reset();
				Session.bProxyDriving = true;
				return true;
			}
		}
	}

	UE_LOG(
		LogDIVE,
		Verbose,
		TEXT("DIVE: Physical pick on '%s' had no device proxy drive and no pawn physical drive backend."),
		*GetNameSafe(HitComponent));

	return false;
}

void FDIVESessionPhysicalDriveOps::UpdateProxyDrive(UDIVESessionSubsystem& Session, const FVector2D& ScreenDelta)
{
	if (!Session.bProxyDriving)
	{
		return;
	}

	switch (Session.ActivePhysicalDriveKind)
	{
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::DeviceProxy:
		if (UObject* ProxyObject = Session.ActiveProxyDrive.GetObject())
		{
			IDIVEProxyDrive::Execute_ApplyProxyDriveDelta(ProxyObject, ScreenDelta);
		}
		else
		{
			ClearProxyDrive(Session);
		}
		break;
	case UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnBridge:
		if (!Session.ActivePawnPhysicalDrive.GetObject())
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
}

void FDIVESessionPhysicalDriveOps::HandleActivePawnPhysicalManualRotatePressed(UDIVESessionSubsystem& Session)
{
	if (!Session.bProxyDriving || Session.ActivePhysicalDriveKind != UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnBridge)
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
	if (!Session.bProxyDriving || Session.ActivePhysicalDriveKind != UDIVESessionSubsystem::EDIVEActivePhysicalDriveKind::PawnBridge)
	{
		return;
	}

	if (UObject* PawnDriveObject = Session.ActivePawnPhysicalDrive.GetObject())
	{
		IDIVEPawnPhysicalDrive::Execute_HandlePawnPhysicalManualRotateReleased(PawnDriveObject);
	}
}
