// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEContextMenuUIComponent.h"

#include "DIVESessionSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/DIVEContextMenuWidget.h"
#include "UI/DIVEValueReadoutWidget.h"

UDIVEContextMenuUIComponent::UDIVEContextMenuUIComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ValueReadoutWidgetClass = UDIVEValueReadoutWidget::StaticClass();
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
	HideValueReadout();
	Super::EndPlay(EndPlayReason);
}

void UDIVEContextMenuUIComponent::BindSessionDelegates()
{
	if (UWorld* World = GetWorld())
	{
		if (UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->OnContextMenuVisibilityChanged.AddUniqueDynamic(this, &UDIVEContextMenuUIComponent::HandleContextMenuVisibilityChanged);
			Subsystem->OnSessionEnded.AddUniqueDynamic(this, &UDIVEContextMenuUIComponent::HandleSessionEnded);
			Subsystem->OnInteractionValueChanged.AddUniqueDynamic(this, &UDIVEContextMenuUIComponent::HandleInteractionValueChanged);

			if (Subsystem->IsContextMenuOpen())
			{
				HandleContextMenuVisibilityChanged(true);
			}
		}
	}
}

void UDIVEContextMenuUIComponent::UnbindSessionDelegates()
{
	if (UWorld* World = GetWorld())
	{
		if (UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->OnContextMenuVisibilityChanged.RemoveDynamic(this, &UDIVEContextMenuUIComponent::HandleContextMenuVisibilityChanged);
			Subsystem->OnSessionEnded.RemoveDynamic(this, &UDIVEContextMenuUIComponent::HandleSessionEnded);
			Subsystem->OnInteractionValueChanged.RemoveDynamic(this, &UDIVEContextMenuUIComponent::HandleInteractionValueChanged);
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
	HideValueReadout();
}

void UDIVEContextMenuUIComponent::HandleContextMenuEntrySelected(
	UDIVEDeviceAction* Action,
	FName TargetKey,
	FName BindingId)
{
	if (UWorld* World = GetWorld())
	{
		if (UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->ExecuteContextMenuAction(Action, TargetKey, BindingId);
		}
	}
}

void UDIVEContextMenuUIComponent::HandleContextMenuDismissed()
{
	if (UWorld* World = GetWorld())
	{
		if (UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			Subsystem->CloseContextMenu();
		}
	}
}

void UDIVEContextMenuUIComponent::HandleInteractionValueChanged(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context,
	float NormalizedValue)
{
	(void)Context;
	if (!IsLocallyControlledOwner())
	{
		return;
	}

	if (!Action)
	{
		if (ValueReadoutWidget)
		{
			ValueReadoutWidget->ClearReadout();
		}
		return;
	}

	EnsureValueReadoutWidget();
	if (ValueReadoutWidget)
	{
		ValueReadoutWidget->SetReadout(Action->GetResolvedDisplayName(), NormalizedValue);
	}
}

void UDIVEContextMenuUIComponent::EnsureValueReadoutWidget()
{
	if (ValueReadoutWidget)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr);
	if (!PlayerController)
	{
		return;
	}

	TSubclassOf<UDIVEValueReadoutWidget> WidgetClass = ValueReadoutWidgetClass;
	if (!*WidgetClass)
	{
		WidgetClass = UDIVEValueReadoutWidget::StaticClass();
	}

	ValueReadoutWidget = CreateWidget<UDIVEValueReadoutWidget>(PlayerController, WidgetClass);
	if (ValueReadoutWidget && !ValueReadoutWidget->IsInViewport())
	{
		ValueReadoutWidget->AddToViewport(ValueReadoutZOrder);
	}
}

void UDIVEContextMenuUIComponent::HideValueReadout()
{
	if (ValueReadoutWidget)
	{
		ValueReadoutWidget->ClearReadout();
		ValueReadoutWidget->RemoveFromParent();
		ValueReadoutWidget = nullptr;
	}
}

void UDIVEContextMenuUIComponent::ShowContextMenu()
{
	APlayerController* PlayerController = Cast<APlayerController>(Cast<APawn>(GetOwner()) ? Cast<APawn>(GetOwner())->GetController() : nullptr);
	if (!PlayerController)
	{
		return;
	}

	UWorld* World = GetWorld();
	UDIVESessionSubsystem* Subsystem = World ? World->GetSubsystem<UDIVESessionSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	if (!ContextMenuWidget)
	{
		TSubclassOf<UDIVEContextMenuWidget> WidgetClass = ContextMenuWidgetClass;
		if (!*WidgetClass)
		{
			WidgetClass = UDIVEContextMenuWidget::StaticClass();
		}
		ContextMenuWidget = CreateWidget<UDIVEContextMenuWidget>(PlayerController, WidgetClass);
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

	ContextMenuWidget->SetStyle(MenuStyle);
	ContextMenuWidget->SetEntries(Subsystem->GetContextMenuEntries());
	ContextMenuWidget->SetScreenPosition(Subsystem->GetContextMenuScreenPosition());

	if (!ContextMenuWidget->IsInViewport())
	{
		ContextMenuWidget->AddToViewport(ViewportZOrder);
	}

	ContextMenuWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
	ContextMenuWidget->SetAlignmentInViewport(FVector2D::ZeroVector);

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
