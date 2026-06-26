// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEOperationsUIComponent.h"

#include "DIVESessionSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/DIVEOperationsListWidget.h"

namespace
{
void ReportOperationResult(const FDIVEOperationResult& Result)
{
	if (Result.bSuccess)
	{
		return;
	}

	const FString Message = Result.Message.ToString();
	UE_LOG(LogTemp, Warning, TEXT("DIVE operation failed: %s"), *Message);

	if (GEngine && !Message.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, Message);
	}
}
}

UDIVEOperationsUIComponent::UDIVEOperationsUIComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UDIVEOperationsUIComponent::BeginPlay()
{
	Super::BeginPlay();
	BindSessionDelegates();
}

void UDIVEOperationsUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSessionDelegates();
	ResetHoldState();

	if (OperationsWidget)
	{
		OperationsWidget->RemoveFromParent();
		OperationsWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UDIVEOperationsUIComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bHoldInProgress)
	{
		return;
	}

	const FDIVEOperationDescriptor* SelectedOperation = GetSelectedOperation();
	if (!SelectedOperation || SelectedOperation->OperationId != HoldOperationId || SelectedOperation->InputMode != EDIVEOperationInputMode::Hold)
	{
		ResetHoldState();
		RefreshOperations();
		return;
	}

	HoldElapsed += DeltaTime;
	if (OperationsWidget)
	{
		const float HoldDuration = ResolveHoldDuration(*SelectedOperation);
		OperationsWidget->SetHoldProgress(HoldDuration > KINDA_SMALL_NUMBER ? FMath::Clamp(HoldElapsed / HoldDuration, 0.f, 1.f) : 1.f);
	}

	const float HoldDuration = ResolveHoldDuration(*SelectedOperation);
	if (HoldElapsed >= HoldDuration)
	{
		if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
			{
				FDIVEOperationResult Result;
				Subsystem->RequestFocusedOperation(HoldOperationId, Result);
				ReportOperationResult(Result);
			}
		}

		ResetHoldState();
		RefreshOperations();
	}
}

void UDIVEOperationsUIComponent::BindSessionDelegates()
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->OnSessionStarted.AddDynamic(this, &UDIVEOperationsUIComponent::HandleSessionStarted);
			Subsystem->OnSessionEnded.AddDynamic(this, &UDIVEOperationsUIComponent::HandleSessionEnded);
			Subsystem->OnFocusChanged.AddDynamic(this, &UDIVEOperationsUIComponent::HandleFocusChanged);

			if (Subsystem->IsSessionActive())
			{
				HandleSessionStarted(Subsystem->GetActiveDeviceHost(), Subsystem->GetActiveInspectable());
				HandleFocusChanged(Subsystem->GetFocusedTarget());
			}
		}
	}
}

void UDIVEOperationsUIComponent::UnbindSessionDelegates()
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->OnSessionStarted.RemoveDynamic(this, &UDIVEOperationsUIComponent::HandleSessionStarted);
			Subsystem->OnSessionEnded.RemoveDynamic(this, &UDIVEOperationsUIComponent::HandleSessionEnded);
			Subsystem->OnFocusChanged.RemoveDynamic(this, &UDIVEOperationsUIComponent::HandleFocusChanged);
		}
	}
}

void UDIVEOperationsUIComponent::HandleSessionStarted(AActor* /*DeviceHost*/, UDIVEInspectableComponent* /*Inspectable*/)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr);
	if (!PlayerController)
	{
		return;
	}

	if (!OperationsWidget)
	{
		OperationsWidget = CreateWidget<UDIVEOperationsListWidget>(PlayerController, UDIVEOperationsListWidget::StaticClass());
	}

	if (OperationsWidget && !OperationsWidget->IsInViewport())
	{
		OperationsWidget->AddToViewport(ViewportZOrder);
	}

	SetComponentTickEnabled(true);
	RefreshOperations();
}

void UDIVEOperationsUIComponent::HandleSessionEnded(EDIVESessionEndReason /*Reason*/, AActor* /*DeviceHost*/)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	ResetHoldState();
	AvailableOperations.Reset();
	SelectedIndex = INDEX_NONE;

	if (OperationsWidget)
	{
		OperationsWidget->RemoveFromParent();
	}

	SetComponentTickEnabled(false);
}

void UDIVEOperationsUIComponent::HandleFocusChanged(const FDIVEFocusTarget& /*FocusTarget*/)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	ResetHoldState();
	RefreshOperations();
}

void UDIVEOperationsUIComponent::RefreshOperations()
{
	AvailableOperations.Reset();
	SelectedIndex = INDEX_NONE;

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->GetAvailableOperations(AvailableOperations);
		}
	}

	if (!AvailableOperations.IsEmpty())
	{
		SelectedIndex = 0;
	}

	if (OperationsWidget)
	{
		const bool bShow = !AvailableOperations.IsEmpty();
		OperationsWidget->SetVisibility(bShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		OperationsWidget->SetOperations(AvailableOperations, SelectedIndex, 0.f);
	}
}

void UDIVEOperationsUIComponent::CycleSelection(int32 Delta)
{
	if (AvailableOperations.IsEmpty())
	{
		return;
	}

	SelectedIndex = (SelectedIndex + Delta) % AvailableOperations.Num();
	if (SelectedIndex < 0)
	{
		SelectedIndex += AvailableOperations.Num();
	}

	ResetHoldState();
	if (OperationsWidget)
	{
		OperationsWidget->SetOperations(AvailableOperations, SelectedIndex, 0.f);
	}
}

void UDIVEOperationsUIComponent::HandleExecutePressed()
{
	if (!IsLocallyControlledOwner() || !AvailableOperations.IsValidIndex(SelectedIndex))
	{
		return;
	}

	const FDIVEOperationDescriptor& Operation = AvailableOperations[SelectedIndex];
	if (Operation.InputMode == EDIVEOperationInputMode::Hold)
	{
		bHoldInProgress = true;
		HoldElapsed = 0.f;
		HoldOperationId = Operation.OperationId;
		if (OperationsWidget)
		{
			OperationsWidget->SetHoldProgress(0.f);
		}

		return;
	}

	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			FDIVEOperationResult Result;
			Subsystem->RequestFocusedOperation(Operation.OperationId, Result);
			ReportOperationResult(Result);
		}
	}

	RefreshOperations();
}

void UDIVEOperationsUIComponent::HandleExecuteReleased()
{
	if (bHoldInProgress)
	{
		ResetHoldState();
		if (OperationsWidget)
		{
			OperationsWidget->SetHoldProgress(0.f);
		}
	}
}

void UDIVEOperationsUIComponent::ResetHoldState()
{
	bHoldInProgress = false;
	HoldElapsed = 0.f;
	HoldOperationId = NAME_None;
}

const FDIVEOperationDescriptor* UDIVEOperationsUIComponent::GetSelectedOperation() const
{
	return AvailableOperations.IsValidIndex(SelectedIndex) ? &AvailableOperations[SelectedIndex] : nullptr;
}

float UDIVEOperationsUIComponent::ResolveHoldDuration(const FDIVEOperationDescriptor& Operation) const
{
	return Operation.HoldDuration > 0.f ? Operation.HoldDuration : DefaultHoldDuration;
}

bool UDIVEOperationsUIComponent::IsLocallyControlledOwner() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}
