// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVESessionChromeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/SharedUmgStyle.h"
#include "Widgets/Layout/Anchors.h"

UDIVESessionChromeWidget::UDIVESessionChromeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
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

FText UDIVESessionChromeWidget::ResolveModeLabel(EDIVESessionInteractionMode InMode) const
{
	switch (InMode)
	{
	case EDIVESessionInteractionMode::Physical:
		return NSLOCTEXT("DIVE", "SessionModePhysical", "Physical");
	case EDIVESessionInteractionMode::Interact:
	default:
		return NSLOCTEXT("DIVE", "SessionModeInteract", "Interact");
	}
}

FLinearColor UDIVESessionChromeWidget::ResolveModeTextColor(EDIVESessionInteractionMode InMode) const
{
	return InMode == EDIVESessionInteractionMode::Physical ? CachedStyle.ModeText : CachedStyle.InteractModeText;
}

void UDIVESessionChromeWidget::RebuildChrome()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	ModePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModePanel"));
	ModePanel->SetPadding(FMargin(CachedStyle.PanelPadding));
	ModePanel->SetBrushColor(CachedStyle.PanelBackground);

	UBorder* PanelOutline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModePanelOutline"));
	PanelOutline->SetPadding(FMargin(1.f));
	PanelOutline->SetBrush(SharedUmgStyle::MakeFlatColorBrush(CachedStyle.PanelBorder, ESlateBrushDrawType::Image));
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
	const bool bHasHint = !CachedStyle.ModeSwitchHint.IsEmpty();
	ModeHint->SetText(CachedStyle.ModeSwitchHint);
	ModeHint->SetColorAndOpacity(CachedStyle.HintText);
	ModeHint->SetVisibility(bHasHint ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

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
