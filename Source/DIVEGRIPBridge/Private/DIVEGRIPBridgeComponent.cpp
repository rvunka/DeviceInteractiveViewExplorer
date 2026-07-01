// Copyright (c) 2026. All Rights Reserved.

#include "DIVEGRIPBridgeComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GRIPLog.h"
#include "Hand/GRIPHandAimComponent.h"
#include "Hand/GRIPHandComponent.h"
#include "Input/DIVEInputComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Physics/GRIPGrabResult.h"
#include "Utils/GRIPComponentResolve.h"
#include "Utils/GRIPGrabDiagnostics.h"

UDIVEGRIPBridgeComponent::UDIVEGRIPBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	SetComponentTickEnabled(false);
}

void UDIVEGRIPBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	ConfigureHandDriveTickOrder();
}

void UDIVEGRIPBridgeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bDriving)
	{
		return;
	}

	UGRIPHandComponent* Hand = ResolveGripHand();
	if (!Hand || !Hand->IsGrabbing())
	{
		return;
	}

	if (Hand->IsManualRotateActive())
	{
		ApplyManualRotationFromMouse();
		return;
	}

	UpdateHandTargetFromCursor();
}

void UDIVEGRIPBridgeComponent::ConfigureHandDriveTickOrder()
{
	AActor* Owner = GetOwner();
	UDIVEInputComponent* DiveInput = Owner ? Owner->FindComponentByClass<UDIVEInputComponent>() : nullptr;
	UGRIPHandComponent* Hand = ResolveGripHand();
	if (!DiveInput || !Hand)
	{
		return;
	}

	if (UGRIPHandAimComponent* Aim = Owner->FindComponentByClass<UGRIPHandAimComponent>())
	{
		DiveInput->PrimaryComponentTick.AddPrerequisite(Aim, Aim->PrimaryComponentTick);
	}

	PrimaryComponentTick.AddPrerequisite(DiveInput, DiveInput->PrimaryComponentTick);
	Hand->PrimaryComponentTick.AddPrerequisite(this, PrimaryComponentTick);
}

void UDIVEGRIPBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (bDriving)
	{
		IDIVEPawnPhysicalDrive::Execute_EndPawnPhysicalDrive(this, false);
	}

	Super::EndPlay(EndPlayReason);
}

void UDIVEGRIPBridgeComponent::SetDriveTickEnabled(const bool bEnabled)
{
	SetComponentTickEnabled(bEnabled);
}

bool UDIVEGRIPBridgeComponent::CanBeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context) const
{
	const UPrimitiveComponent* HitComponent = Context.HitComponent;
	if (!HitComponent || !HitComponent->IsSimulatingPhysics())
	{
		return false;
	}

	const UGRIPHandComponent* Hand = ResolveGripHand();
	return Hand && Hand->CanGripPrimitive(HitComponent);
}

UGRIPHandComponent* UDIVEGRIPBridgeComponent::ResolveGripHand() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return GRIPComponentResolve::FindComponentByNameOrClass<UGRIPHandComponent>(Owner, GripHandComponentName);
}

APlayerController* UDIVEGRIPBridgeComponent::ResolvePlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn ? OwnerPawn->GetController<APlayerController>() : nullptr;
}

bool UDIVEGRIPBridgeComponent::TryGetCursorScreenPosition(FVector2D& OutScreenPosition) const
{
	if (const APlayerController* PlayerController = ResolvePlayerController())
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		if (PlayerController->GetMousePosition(MouseX, MouseY))
		{
			OutScreenPosition = FVector2D(MouseX, MouseY);
			return true;
		}
	}

	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* ViewportClient = World->GetGameViewport())
		{
			FVector2D ViewportPosition;
			if (ViewportClient->GetMousePosition(ViewportPosition))
			{
				OutScreenPosition = ViewportPosition;
				return true;
			}
		}
	}

	return false;
}

bool UDIVEGRIPBridgeComponent::ResolveHandTargetFromScreen(const FVector2D& ScreenPosition, FVector& OutWorldLocation) const
{
	APlayerController* PlayerController = ResolvePlayerController();
	if (!PlayerController || GrabHoldDistance <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (!UGameplayStatics::DeprojectScreenToWorld(PlayerController, ScreenPosition, WorldOrigin, WorldDirection))
	{
		return false;
	}

	const float RayLength = WorldDirection.Size();
	if (RayLength <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	WorldDirection /= RayLength;
	OutWorldLocation = WorldOrigin + WorldDirection * GrabHoldDistance;
	return true;
}

void UDIVEGRIPBridgeComponent::UpdateHandTargetFromCursor()
{
	UGRIPHandComponent* Hand = ResolveGripHand();
	APlayerController* PlayerController = ResolvePlayerController();
	if (!Hand || !PlayerController || !Hand->IsGrabbing())
	{
		return;
	}

	FVector2D ScreenPosition;
	if (!TryGetCursorScreenPosition(ScreenPosition))
	{
		return;
	}

	FVector HandLocation = FVector::ZeroVector;
	if (!ResolveHandTargetFromScreen(ScreenPosition, HandLocation))
	{
		return;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	Hand->SetHandWorldTransform(FTransform(ViewRotation, HandLocation));
}

void UDIVEGRIPBridgeComponent::ApplyManualRotationFromMouse()
{
	UGRIPHandComponent* Hand = ResolveGripHand();
	APlayerController* PlayerController = ResolvePlayerController();
	if (!Hand || !PlayerController || !Hand->IsManualRotateActive())
	{
		return;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	float MouseDeltaX = 0.f;
	float MouseDeltaY = 0.f;
	PlayerController->GetInputMouseDelta(MouseDeltaX, MouseDeltaY);
	if (!FMath::IsNearlyZero(MouseDeltaX) || !FMath::IsNearlyZero(MouseDeltaY))
	{
		Hand->AddManualRotationInput(MouseDeltaX, MouseDeltaY, ViewRotation);
	}
}

bool UDIVEGRIPBridgeComponent::TryEnterManualRotateMouseCapture()
{
	if (bManualRotateMouseCaptureActive)
	{
		return true;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!PlayerController || !ViewportClient)
	{
		return false;
	}

	bHasPreservedCursorScreenPositionDuringRotate = TryGetCursorScreenPosition(PreservedCursorScreenPositionDuringRotate);
	bPreservedShowMouseCursorDuringRotate = PlayerController->bShowMouseCursor;
	PreservedMouseCaptureModeDuringRotate = ViewportClient->GetMouseCaptureMode();

	PlayerController->SetInputMode(FInputModeGameOnly());
	PlayerController->bShowMouseCursor = false;
	ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently);

	bManualRotateMouseCaptureActive = true;
	return true;
}

void UDIVEGRIPBridgeComponent::ExitManualRotateMouseCapture()
{
	if (!bManualRotateMouseCaptureActive)
	{
		return;
	}

	const bool bResumeDrag = bDriving;

	APlayerController* PlayerController = ResolvePlayerController();
	UGameViewportClient* ViewportClient = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (PlayerController && ViewportClient)
	{
		if (bResumeDrag)
		{
			if (AActor* Owner = GetOwner())
			{
				if (UDIVEInputComponent* DiveInput = Owner->FindComponentByClass<UDIVEInputComponent>())
				{
					DiveInput->ReapplySessionInputMode();
				}
			}

			ViewportClient->SetMouseCaptureMode(EMouseCaptureMode::CaptureDuringMouseDown);

			if (bHasPreservedCursorScreenPositionDuringRotate)
			{
				PlayerController->SetMouseLocation(
					FMath::RoundToInt(PreservedCursorScreenPositionDuringRotate.X),
					FMath::RoundToInt(PreservedCursorScreenPositionDuringRotate.Y));
			}
		}
		else
		{
			PlayerController->bShowMouseCursor = bPreservedShowMouseCursorDuringRotate;
			ViewportClient->SetMouseCaptureMode(PreservedMouseCaptureModeDuringRotate);
		}
	}

	bManualRotateMouseCaptureActive = false;
	bHasPreservedCursorScreenPositionDuringRotate = false;
}

void UDIVEGRIPBridgeComponent::SuspendGripAimUpdates()
{
	if (bSuspendedAimUpdates)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (UGRIPHandAimComponent* Aim = Owner ? Owner->FindComponentByClass<UGRIPHandAimComponent>() : nullptr)
	{
		CachedAimComponent = Aim;
		Aim->SetAimSuppressed(true);
	}

	bSuspendedAimUpdates = true;
}

void UDIVEGRIPBridgeComponent::RestoreGripAimUpdates()
{
	if (!bSuspendedAimUpdates)
	{
		return;
	}

	if (CachedAimComponent)
	{
		CachedAimComponent->SetAimSuppressed(false);
		CachedAimComponent = nullptr;
	}

	bSuspendedAimUpdates = false;
}

bool UDIVEGRIPBridgeComponent::BeginPawnPhysicalDrive_Implementation(const FDIVEProxyDriveContext& Context)
{
	UGRIPHandComponent* Hand = ResolveGripHand();
	if (!Hand || !Context.HitComponent || Context.PickHit.GetComponent() != Context.HitComponent)
	{
		return false;
	}

	APlayerController* PlayerController = ResolvePlayerController();
	if (!PlayerController)
	{
		return false;
	}

	FRotator ViewRotation = FRotator::ZeroRotator;
	FVector ViewLocation = FVector::ZeroVector;
	PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);

	const FHitResult& Hit = Context.PickHit;
	const FVector GrabPoint = Hit.bBlockingHit ? Hit.ImpactPoint : Hit.Location;

	FVector WorldOrigin = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	if (UGameplayStatics::DeprojectScreenToWorld(PlayerController, Context.ScreenPosition, WorldOrigin, WorldDirection))
	{
		const float RayLength = WorldDirection.Size();
		if (RayLength > KINDA_SMALL_NUMBER)
		{
			WorldDirection /= RayLength;
			GrabHoldDistance = FVector::DotProduct(GrabPoint - WorldOrigin, WorldDirection);
		}
	}

	if (GrabHoldDistance <= KINDA_SMALL_NUMBER)
	{
		GrabHoldDistance = FVector::Distance(ViewLocation, GrabPoint);
	}

	SuspendGripAimUpdates();
	Hand->SetHandWorldTransform(FTransform(ViewRotation, GrabPoint));

	const EGRIPGrabResult Result = Hand->TryGrabFromHit(Hit);
	if (Result != EGRIPGrabResult::Success)
	{
		RestoreGripAimUpdates();
		GrabHoldDistance = 0.f;
		UE_LOG(LogGRIP, Warning, TEXT("DIVE GRIP bridge: BeginPawnPhysicalDrive failed: %s"),
			*UGRIPGrabDiagnostics::GetGrabResultLogString(Result));
		return false;
	}

	bDriving = true;
	SetDriveTickEnabled(true);
	return true;
}

void UDIVEGRIPBridgeComponent::ApplyGrabHoldDistanceScroll(const float WheelDelta)
{
	if (!bDriving || FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}

	UGRIPHandComponent* Hand = ResolveGripHand();
	if (!Hand || !Hand->IsGrabbing() || Hand->IsManualRotateActive())
	{
		return;
	}

	const float DeltaCm = WheelDelta * Hand->GetGrabHoldDistanceScrollStepCm();
	GrabHoldDistance = FMath::Clamp(
		GrabHoldDistance + DeltaCm,
		Hand->MinGrabHoldDistance,
		Hand->MaxGrabHoldDistance);
	UpdateHandTargetFromCursor();
}

void UDIVEGRIPBridgeComponent::ApplyPawnPhysicalDriveDelta_Implementation(FVector2D ScreenDelta)
{
	(void)ScreenDelta;
}

void UDIVEGRIPBridgeComponent::HandlePawnPhysicalManualRotatePressed_Implementation()
{
	if (!bDriving)
	{
		return;
	}

	UGRIPHandComponent* Hand = ResolveGripHand();
	if (!Hand || !Hand->IsGrabbing() || Hand->IsManualRotateActive())
	{
		return;
	}

	if (Hand->BeginManualRotate())
	{
		if (!TryEnterManualRotateMouseCapture())
		{
			Hand->EndManualRotate();
		}
	}
}

void UDIVEGRIPBridgeComponent::HandlePawnPhysicalManualRotateReleased_Implementation()
{
	ExitManualRotateMouseCapture();

	if (UGRIPHandComponent* Hand = ResolveGripHand())
	{
		if (Hand->IsManualRotateActive())
		{
			Hand->EndManualRotate();
		}
	}
}

void UDIVEGRIPBridgeComponent::EndPawnPhysicalDrive_Implementation(bool bCommit)
{
	ExitManualRotateMouseCapture();

	if (UGRIPHandComponent* Hand = ResolveGripHand())
	{
		if (Hand->IsManualRotateActive())
		{
			Hand->EndManualRotate();
		}

		if (Hand->IsGrabbing())
		{
			Hand->ReleaseGrab(bCommit);
		}
	}

	RestoreGripAimUpdates();
	bDriving = false;
	GrabHoldDistance = 0.f;
	SetDriveTickEnabled(false);
}
