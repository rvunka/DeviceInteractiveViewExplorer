// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEValueReadoutWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "UI/SharedUmgStyle.h"

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

void UDIVEValueReadoutWidget::SetReadout(FText Label, float NormalizedValue)
{
	if (LabelText)
	{
		LabelText->SetText(FText::Format(
			NSLOCTEXT("DIVE", "ValueReadoutFormat", "{0}: {1}"),
			Label,
			FText::AsNumber(NormalizedValue)));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDIVEValueReadoutWidget::ClearReadout()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
