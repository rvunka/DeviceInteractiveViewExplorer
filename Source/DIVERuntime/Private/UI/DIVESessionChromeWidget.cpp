// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVESessionChromeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/CoreStyle.h"

namespace
{
FSlateBrush MakeFlatColorBrush(const FLinearColor& Color)
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.TintColor = FSlateColor(Color);
	Brush.Margin = FMargin(0.f);
	Brush.ImageSize = FVector2D(1.f, 1.f);
	return Brush;
}
} // namespace

UDIVESessionChromeWidget::UDIVESessionChromeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UDIVESessionChromeWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RebuildChrome();
}

void UDIVESessionChromeWidget::SetStyle(const FDIVESessionChromeStyle& InStyle)
{
	CachedStyle = InStyle;
	RebuildChrome();
}

void UDIVESessionChromeWidget::SetInteractionMode(EDIVESessionInteractionMode InMode)
{
	CachedMode = InMode;

	if (ModeLabel)
	{
		ModeLabel->SetText(ResolveModeLabel(InMode));
		ModeLabel->SetColorAndOpacity(ResolveModeTextColor(InMode));
	}
}

void UDIVESessionChromeWidget::SetModeHintVisible(bool bInVisible)
{
	bShowModeHint = bInVisible;

	if (ModeHint)
	{
		ModeHint->SetVisibility(bInVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

FText UDIVESessionChromeWidget::ResolveModeLabel(EDIVESessionInteractionMode InMode) const
{
	switch (InMode)
	{
	case EDIVESessionInteractionMode::Physical:
		return NSLOCTEXT("DIVE", "SessionModePhysical", "Physical");
	default:
		return NSLOCTEXT("DIVE", "SessionModeDefault", "Default");
	}
}

FLinearColor UDIVESessionChromeWidget::ResolveModeTextColor(EDIVESessionInteractionMode InMode) const
{
	return InMode == EDIVESessionInteractionMode::Physical ? CachedStyle.ModeText : CachedStyle.DefaultModeText;
}

void UDIVESessionChromeWidget::RebuildChrome()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	ModePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModePanel"));
	ModePanel->SetPadding(FMargin(CachedStyle.PanelPadding));
	ModePanel->SetBrushColor(CachedStyle.PanelBackground);

	UBorder* PanelOutline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModePanelOutline"));
	PanelOutline->SetPadding(FMargin(1.f));
	PanelOutline->SetBrush(MakeFlatColorBrush(CachedStyle.PanelBorder));
	PanelOutline->SetContent(ModePanel);

	UVerticalBox* ContentBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModeContent"));
	ModePanel->SetContent(ContentBox);

	ModeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModeLabel"));
	ModeLabel->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), CachedStyle.ModeFontSize));
	ModeLabel->SetText(ResolveModeLabel(CachedMode));
	ModeLabel->SetColorAndOpacity(ResolveModeTextColor(CachedMode));

	if (UVerticalBoxSlot* ModeSlot = ContentBox->AddChildToVerticalBox(ModeLabel))
	{
		ModeSlot->SetPadding(FMargin(0.f));
	}

	ModeHint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModeHint"));
	ModeHint->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), CachedStyle.HintFontSize));
	ModeHint->SetText(NSLOCTEXT("DIVE", "SessionModeHint", "Tab — switch mode"));
	ModeHint->SetColorAndOpacity(CachedStyle.HintText);
	ModeHint->SetVisibility(bShowModeHint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (UVerticalBoxSlot* HintSlot = ContentBox->AddChildToVerticalBox(ModeHint))
	{
		HintSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelOutline))
	{
		PanelSlot->SetAnchors(FAnchors(1.f, 0.f, 1.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(1.f, 0.f));
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetPosition(FVector2D(-CachedStyle.Margin, CachedStyle.Margin));
	}
}
