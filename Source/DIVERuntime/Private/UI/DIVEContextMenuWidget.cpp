// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEContextMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameViewportClient.h"
#include "InputCoreTypes.h"

void UDIVEContextMenuRowHost::HandleClicked()
{
	if (UDIVEContextMenuWidget* Menu = OwnerWidget.Get())
	{
		Menu->HandleEntryClicked(ActionId);
	}
}

void UDIVEContextMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	EntryList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntryList"));
	if (UCanvasPanelSlot* ListSlot = RootCanvas->AddChildToCanvas(EntryList))
	{
		ListSlot->SetAutoSize(true);
		ListSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		ListSlot->SetAlignment(FVector2D(0.f, 0.f));
	}
}

FReply UDIVEContextMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		OnDismissed.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UDIVEContextMenuWidget::SetEntries(const TArray<FDIVEContextMenuEntry>& Entries)
{
	CachedEntries = Entries;
	RebuildList();
}

void UDIVEContextMenuWidget::SetScreenPosition(const FVector2D& InScreenPosition)
{
	CachedScreenPosition = ClampPositionToViewport(InScreenPosition);

	if (EntryList && EntryList->Slot)
	{
		if (UCanvasPanelSlot* ListSlot = Cast<UCanvasPanelSlot>(EntryList->Slot))
		{
			ListSlot->SetPosition(CachedScreenPosition);
		}
	}
}

FVector2D UDIVEContextMenuWidget::ClampPositionToViewport(const FVector2D& ScreenPosition) const
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	else
	{
		ViewportSize = FVector2D(1920.f, 1080.f);
	}

	const float Margin = 8.f;
	const float EstimatedWidth = 220.f;
	const float EstimatedHeight = FMath::Max(48.f, CachedEntries.Num() * 36.f);

	const float MaxX = FMath::Max(Margin, ViewportSize.X - EstimatedWidth - Margin);
	const float MaxY = FMath::Max(Margin, ViewportSize.Y - EstimatedHeight - Margin);

	return FVector2D(
		FMath::Clamp(ScreenPosition.X, Margin, MaxX),
		FMath::Clamp(ScreenPosition.Y, Margin, MaxY));
}

void UDIVEContextMenuWidget::HandleEntryClicked(FName ActionId)
{
	if (!ActionId.IsNone())
	{
		OnEntrySelected.Broadcast(ActionId);
	}
}

void UDIVEContextMenuWidget::RebuildList()
{
	if (!EntryList || !WidgetTree)
	{
		return;
	}

	EntryList->ClearChildren();
	RowHosts.Reset();

	for (const FDIVEContextMenuEntry& Entry : CachedEntries)
	{
		UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		RowButton->SetIsEnabled(Entry.bEnabled);

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Entry.DisplayName);
		Label->SetColorAndOpacity(FSlateColor(Entry.bEnabled ? FLinearColor::White : FLinearColor(0.45f, 0.45f, 0.45f)));
		RowButton->SetContent(Label);

		if (Entry.bEnabled)
		{
			UDIVEContextMenuRowHost* RowHost = NewObject<UDIVEContextMenuRowHost>(this);
			RowHost->ActionId = Entry.ActionId;
			RowHost->OwnerWidget = this;
			RowHosts.Add(RowHost);
			RowButton->OnClicked.AddDynamic(RowHost, &UDIVEContextMenuRowHost::HandleClicked);
		}

		if (UVerticalBoxSlot* RowSlot = EntryList->AddChildToVerticalBox(RowButton))
		{
			RowSlot->SetPadding(FMargin(0.f, 2.f));
		}
	}

	SetScreenPosition(CachedScreenPosition);
}
