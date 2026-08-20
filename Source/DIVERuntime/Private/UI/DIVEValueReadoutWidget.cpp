// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEValueReadoutWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"

UDIVEValueReadoutWidget::UDIVEValueReadoutWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDIVEValueReadoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	PanelBorder->SetPadding(FMargin(10.f, 6.f));
	PanelBorder->SetBrushColor(FLinearColor(0.05f, 0.05f, 0.05f, 0.75f));
	WidgetTree->RootWidget = PanelBorder;

	LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LabelText"));
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	PanelBorder->SetContent(LabelText);
}

void UDIVEValueReadoutWidget::SetReadout(FText Label, const FDIVEInteractionValue& Value)
{
	if (LabelText)
	{
		LabelText->SetText(DIVE::FormatInteractionValueReadout(Label, Value));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDIVEValueReadoutWidget::SetNormalizedReadout(FText Label, const float NormalizedValue)
{
	FDIVEInteractionValue Value;
	Value.Normalized = FMath::Clamp(NormalizedValue, 0.f, 1.f);
	SetReadout(MoveTemp(Label), Value);
}

void UDIVEValueReadoutWidget::ClearReadout()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
