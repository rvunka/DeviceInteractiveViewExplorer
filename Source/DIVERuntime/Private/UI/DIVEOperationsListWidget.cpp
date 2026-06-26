// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEOperationsListWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

void UDIVEOperationsListWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	RootList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootList"));
	WidgetTree->RootWidget = RootList;

	OperationList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OperationList"));
	if (UVerticalBoxSlot* OperationSlot = RootList->AddChildToVerticalBox(OperationList))
	{
		OperationSlot->SetPadding(FMargin(0.f, 4.f));
	}

	SelectedHoldProgress = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HoldProgress"));
	SelectedHoldProgress->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* ProgressSlot = RootList->AddChildToVerticalBox(SelectedHoldProgress))
	{
		ProgressSlot->SetPadding(FMargin(0.f, 4.f));
	}
}

void UDIVEOperationsListWidget::SetOperations(
	const TArray<FDIVEOperationDescriptor>& Operations,
	int32 SelectedIndex,
	float HoldProgress)
{
	CachedOperations = Operations;
	CachedSelectedIndex = SelectedIndex;
	CachedHoldProgress = HoldProgress;
	RebuildList();
}

void UDIVEOperationsListWidget::SetHoldProgress(float HoldProgress)
{
	CachedHoldProgress = FMath::Clamp(HoldProgress, 0.f, 1.f);
	if (SelectedHoldProgress)
	{
		SelectedHoldProgress->SetPercent(CachedHoldProgress);
	}
}

void UDIVEOperationsListWidget::RebuildList()
{
	if (!OperationList)
	{
		return;
	}

	OperationList->ClearChildren();

	for (int32 Index = 0; Index < CachedOperations.Num(); ++Index)
	{
		const FDIVEOperationDescriptor& Operation = CachedOperations[Index];
		const bool bSelected = Index == CachedSelectedIndex;

		UBorder* RowBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		RowBorder->SetPadding(FMargin(8.f, 4.f));
		RowBorder->SetBrushColor(bSelected ? FLinearColor(0.08f, 0.18f, 0.32f, 0.92f) : FLinearColor(0.02f, 0.02f, 0.02f, 0.75f));

		UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FString Prefix;
		if (Operation.InputMode == EDIVEOperationInputMode::Hold)
		{
			Prefix = bSelected ? TEXT("> [Hold F] ") : TEXT("  [Hold F] ");
		}
		else
		{
			Prefix = bSelected ? TEXT("> [F] ") : TEXT("  [F] ");
		}

		Label->SetText(FText::FromString(Prefix + Operation.DisplayName.ToString()));
		Label->SetColorAndOpacity(FSlateColor(bSelected ? FLinearColor::White : FLinearColor(0.82f, 0.82f, 0.82f)));
		RowBorder->SetContent(Label);
		OperationList->AddChildToVerticalBox(RowBorder);
	}

	if (SelectedHoldProgress)
	{
		const bool bShowHold = CachedSelectedIndex != INDEX_NONE
			&& CachedOperations.IsValidIndex(CachedSelectedIndex)
			&& CachedOperations[CachedSelectedIndex].InputMode == EDIVEOperationInputMode::Hold;
		SelectedHoldProgress->SetVisibility(bShowHold ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		SelectedHoldProgress->SetPercent(CachedHoldProgress);
	}
}
