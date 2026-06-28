// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEContextMenuUIComponent.h"

#include "DIVESessionSubsystem.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/DIVEContextMenuWidget.h"

UDIVEContextMenuUIComponent::UDIVEContextMenuUIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDIVEContextMenuUIComponent::BeginPlay()
{
	Super::BeginPlay();
	BindSessionDelegates();
}

void UDIVEContextMenuUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindSessionDelegates();
	HideContextMenu();
	Super::EndPlay(EndPlayReason);
}

void UDIVEContextMenuUIComponent::BindSessionDelegates()
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->OnContextMenuVisibilityChanged.AddDynamic(this, &UDIVEContextMenuUIComponent::HandleContextMenuVisibilityChanged);
			Subsystem->OnSessionEnded.AddDynamic(this, &UDIVEContextMenuUIComponent::HandleSessionEnded);

			if (Subsystem->IsContextMenuOpen())
			{
				HandleContextMenuVisibilityChanged(true);
			}
		}
	}
}

void UDIVEContextMenuUIComponent::UnbindSessionDelegates()
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->OnContextMenuVisibilityChanged.RemoveDynamic(this, &UDIVEContextMenuUIComponent::HandleContextMenuVisibilityChanged);
			Subsystem->OnSessionEnded.RemoveDynamic(this, &UDIVEContextMenuUIComponent::HandleSessionEnded);
		}
	}
}

void UDIVEContextMenuUIComponent::HandleContextMenuVisibilityChanged(bool bIsOpen)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (bIsOpen)
	{
		ShowContextMenu();
	}
	else
	{
		HideContextMenu();
	}
}

void UDIVEContextMenuUIComponent::HandleSessionEnded(EDIVESessionEndReason /*Reason*/, AActor* /*DeviceHost*/)
{
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	HideContextMenu();
}

void UDIVEContextMenuUIComponent::HandleContextMenuEntrySelected(FName ActionId)
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->ExecuteContextMenuAction(ActionId);
		}
	}
}

void UDIVEContextMenuUIComponent::HandleContextMenuDismissed()
{
	if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
	{
		if (UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->CloseContextMenu();
		}
	}
}

void UDIVEContextMenuUIComponent::ShowContextMenu()
{
	APlayerController* PlayerController = Cast<APlayerController>(Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr);
	if (!PlayerController)
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UDIVESessionSubsystem* Subsystem = GameInstance ? GameInstance->GetSubsystem<UDIVESessionSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	if (!ContextMenuWidget)
	{
		ContextMenuWidget = CreateWidget<UDIVEContextMenuWidget>(PlayerController, UDIVEContextMenuWidget::StaticClass());
		if (ContextMenuWidget)
		{
			ContextMenuWidget->OnEntrySelected.AddDynamic(this, &UDIVEContextMenuUIComponent::HandleContextMenuEntrySelected);
			ContextMenuWidget->OnDismissed.AddDynamic(this, &UDIVEContextMenuUIComponent::HandleContextMenuDismissed);
		}
	}

	if (!ContextMenuWidget)
	{
		return;
	}

	ContextMenuWidget->SetEntries(Subsystem->GetContextMenuEntries());
	ContextMenuWidget->SetScreenPosition(Subsystem->GetContextMenuScreenPosition());

	if (!ContextMenuWidget->IsInViewport())
	{
		ContextMenuWidget->AddToViewport(ViewportZOrder);
	}

	ContextMenuWidget->SetVisibility(ESlateVisibility::Visible);
	ContextMenuWidget->SetKeyboardFocus();
}

void UDIVEContextMenuUIComponent::HideContextMenu()
{
	if (ContextMenuWidget)
	{
		ContextMenuWidget->RemoveFromParent();
		ContextMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

bool UDIVEContextMenuUIComponent::IsLocallyControlledOwner() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}
