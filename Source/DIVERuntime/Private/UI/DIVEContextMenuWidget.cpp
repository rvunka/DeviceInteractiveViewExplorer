// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEContextMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "DIVEDeviceAction.h"
#include "Engine/GameViewportClient.h"
#include "InputCoreTypes.h"
#include "UI/SharedUmgStyle.h"

namespace
{
constexpr ESlateBrushDrawType::Type kDiveFlatBrushDrawAs = ESlateBrushDrawType::Image;

FSlateBrush MakeDiveFlatColorBrush(const FLinearColor& Color)
{
	return SharedUmgStyle::MakeFlatColorBrush(Color, kDiveFlatBrushDrawAs);
}

float GetViewportScaleSafe(const UObject* WorldContextObject)
{
	const float Scale = WorldContextObject ? UWidgetLayoutLibrary::GetViewportScale(WorldContextObject) : 1.f;
	return Scale > KINDA_SMALL_NUMBER ? Scale : 1.f;
}

float SnapSlateUnitsToPhysicalPixel(float Value, float ViewportScale)
{
	return FMath::RoundToFloat(Value * ViewportScale) / ViewportScale;
}

float OnePhysicalPixelInSlateUnits(float ViewportScale)
{
	return 1.f / ViewportScale;
}

FButtonStyle MakeRowButtonStyle(const FDIVEContextMenuStyle& Style)
{
	FButtonStyle ButtonStyle;
	ButtonStyle.Normal = MakeDiveFlatColorBrush(FLinearColor::Transparent);
	ButtonStyle.Hovered = MakeDiveFlatColorBrush(Style.RowHoverBackground);
	ButtonStyle.Pressed = MakeDiveFlatColorBrush(Style.RowPressedBackground);
	ButtonStyle.Disabled = MakeDiveFlatColorBrush(FLinearColor::Transparent);
	ButtonStyle.NormalPadding = FMargin(Style.RowHorizontalPadding, 0.f);
	ButtonStyle.PressedPadding = FMargin(Style.RowHorizontalPadding, 0.f);
	return ButtonStyle;
}

FButtonStyle MakeTransparentDismissButtonStyle()
{
	FButtonStyle ButtonStyle;
	const FSlateBrush TransparentBrush = MakeDiveFlatColorBrush(FLinearColor::Transparent);
	ButtonStyle.Normal = TransparentBrush;
	ButtonStyle.Hovered = TransparentBrush;
	ButtonStyle.Pressed = TransparentBrush;
	ButtonStyle.Disabled = TransparentBrush;
	ButtonStyle.NormalPadding = FMargin(0.f);
	ButtonStyle.PressedPadding = FMargin(0.f);
	return ButtonStyle;
}

FName ResolveDiveMenuTypefaceName(EDIVEContextMenuTypeface Typeface)
{
	switch (Typeface)
	{
	case EDIVEContextMenuTypeface::Bold:
		return FName(TEXT("Bold"));
	case EDIVEContextMenuTypeface::Light:
		return FName(TEXT("Light"));
	default:
		return FName(TEXT("Regular"));
	}
}

FText FormatRowLabel(const FText& DisplayName, const bool bChecked)
{
	if (!bChecked)
	{
		return DisplayName;
	}

	return FText::Format(
		NSLOCTEXT("DIVE", "ContextMenuCheckedRow", "✓ {0}"),
		DisplayName);
}

} // namespace

UDIVEContextMenuWidget::UDIVEContextMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UDIVEContextMenuActionButton::HandleClicked()
{
	if (UDIVEContextMenuWidget* Menu = OwnerWidget.Get())
	{
		Menu->HandleEntryClicked(Action, TargetKey);
	}
}

void UDIVEContextMenuWidget::HandleDismissCaptureClicked()
{
	OnDismissed.Broadcast();
}

void UDIVEContextMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	WidgetTree->RootWidget = RootCanvas;

	DismissCapture = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DismissCapture"));
	DismissCapture->SetStyle(MakeTransparentDismissButtonStyle());
	DismissCapture->OnClicked.AddDynamic(this, &UDIVEContextMenuWidget::HandleDismissCaptureClicked);

	OuterFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OuterFrame"));
	PanelBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBorder"));
	PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	EntryList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("EntryList"));
	PanelSize->AddChild(EntryList);
	PanelBorder->AddChild(PanelSize);
	OuterFrame->AddChild(PanelBorder);

	if (UCanvasPanelSlot* DismissSlot = RootCanvas->AddChildToCanvas(DismissCapture))
	{
		DismissSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		DismissSlot->SetOffsets(FMargin(0.f));
		DismissSlot->SetAlignment(FVector2D(0.f, 0.f));
	}

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(OuterFrame))
	{
		PanelSlot->SetAutoSize(true);
		PanelSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		PanelSlot->SetAlignment(FVector2D(0.f, 0.f));
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

FReply UDIVEContextMenuWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton().IsValid() && OuterFrame)
	{
		const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
		const FGeometry PanelGeometry = OuterFrame->GetCachedGeometry();
		if (!PanelGeometry.IsUnderLocation(ScreenPosition))
		{
			OnDismissed.Broadcast();
			return FReply::Handled();
		}
	}

	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UDIVEContextMenuWidget::UpdateDismissCaptureSize()
{
	if (!DismissCapture)
	{
		return;
	}

	FVector2D ViewportSize(1920.f, 1080.f);
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}

	if (UCanvasPanelSlot* DismissSlot = Cast<UCanvasPanelSlot>(DismissCapture->Slot))
	{
		DismissSlot->SetSize(FVector2D(ViewportSize.X, ViewportSize.Y));
	}
}

void UDIVEContextMenuWidget::SetStyle(const FDIVEContextMenuStyle& InStyle)
{
	CachedStyle = InStyle;
	RebuildList();
}

void UDIVEContextMenuWidget::SetEntries(const TArray<FDIVEContextMenuEntry>& Entries)
{
	CachedEntries = Entries;
	RebuildList();
}

void UDIVEContextMenuWidget::SetScreenPosition(const FVector2D& InScreenPosition)
{
	CachedScreenPosition = ClampPositionToViewport(InScreenPosition);
	UpdateDismissCaptureSize();

	if (OuterFrame && OuterFrame->Slot)
	{
		if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(OuterFrame->Slot))
		{
			const float ViewportScale = GetViewportScaleSafe(this);
			PanelSlot->SetPosition(FVector2D(
				SnapSlateUnitsToPhysicalPixel(CachedScreenPosition.X, ViewportScale),
				SnapSlateUnitsToPhysicalPixel(CachedScreenPosition.Y, ViewportScale)));
		}
	}
}

float UDIVEContextMenuWidget::GetSectionDividerHeight(const FText& Header) const
{
	const float ViewportScale = GetViewportScaleSafe(this);
	const float SectionSpacing = SnapSlateUnitsToPhysicalPixel(CachedStyle.SectionSpacing, ViewportScale);
	const float SeparatorHeight = OnePhysicalPixelInSlateUnits(ViewportScale);
	float Height = SectionSpacing * 2.f + SeparatorHeight;
	if (!Header.IsEmpty())
	{
		Height += SnapSlateUnitsToPhysicalPixel(CachedStyle.SectionHeaderHeight, ViewportScale);
	}
	return Height;
}

float UDIVEContextMenuWidget::GetEstimatedMenuHeight() const
{
	const float ViewportScale = GetViewportScaleSafe(this);
	const float RowHeight = SnapSlateUnitsToPhysicalPixel(CachedStyle.RowHeight, ViewportScale);

	float Height = SnapSlateUnitsToPhysicalPixel(CachedStyle.PanelPadding * 2.f + 2.f, ViewportScale);

	for (const FDIVEContextMenuEntry& Entry : CachedEntries)
	{
		if (Entry.bIsSeparator)
		{
			Height += GetSectionDividerHeight(Entry.DisplayName);
		}
		else
		{
			Height += RowHeight;
		}
	}

	return Height;
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

	const float Margin = 6.f;
	const float EstimatedWidth = CachedStyle.MenuWidth + CachedStyle.PanelPadding * 2.f + 2.f;
	const float EstimatedHeight = GetEstimatedMenuHeight();

	const float MaxX = FMath::Max(Margin, ViewportSize.X - EstimatedWidth - Margin);
	const float MaxY = FMath::Max(Margin, ViewportSize.Y - EstimatedHeight - Margin);

	const float ViewportScale = GetViewportScaleSafe(this);
	const FVector2D ClampedPosition(
		FMath::Clamp(ScreenPosition.X, Margin, MaxX),
		FMath::Clamp(ScreenPosition.Y, Margin, MaxY));

	return FVector2D(
		SnapSlateUnitsToPhysicalPixel(ClampedPosition.X, ViewportScale),
		SnapSlateUnitsToPhysicalPixel(ClampedPosition.Y, ViewportScale));
}

FSlateFontInfo UDIVEContextMenuWidget::ResolveRowFont() const
{
	return SharedUmgStyle::ResolveMenuFont(
		CachedStyle.RowFont,
		ResolveDiveMenuTypefaceName(CachedStyle.RowTypeface),
		CachedStyle.RowFontSize,
		24);
}

FSlateFontInfo UDIVEContextMenuWidget::ResolveSectionHeaderFont() const
{
	return SharedUmgStyle::ResolveMenuFont(
		CachedStyle.RowFont,
		ResolveDiveMenuTypefaceName(CachedStyle.RowTypeface),
		CachedStyle.SectionHeaderFontSize,
		24);
}

void UDIVEContextMenuWidget::HandleEntryClicked(UDIVEDeviceAction* Action, FName TargetKey)
{
	if (Action)
	{
		OnEntrySelected.Broadcast(Action, TargetKey);
	}
}

void UDIVEContextMenuWidget::AddSectionDivider(const FText& Header)
{
	if (!EntryList || !WidgetTree)
	{
		return;
	}

	const float ViewportScale = GetViewportScaleSafe(this);
	const float SectionSpacing = SnapSlateUnitsToPhysicalPixel(CachedStyle.SectionSpacing, ViewportScale);
	const float SeparatorHeight = OnePhysicalPixelInSlateUnits(ViewportScale);
	const float HeaderHeight = Header.IsEmpty()
		? 0.f
		: SnapSlateUnitsToPhysicalPixel(CachedStyle.SectionHeaderHeight, ViewportScale);
	const float TotalHeight = GetSectionDividerHeight(Header);

	USizeBox* DividerSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	DividerSize->SetHeightOverride(TotalHeight);
	DividerSize->SetMinDesiredHeight(TotalHeight);
	DividerSize->SetWidthOverride(CachedStyle.MenuWidth);

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

	USizeBox* SpacerTop = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SpacerTop->SetHeightOverride(SectionSpacing);
	SpacerTop->SetMinDesiredHeight(SectionSpacing);
	Column->AddChildToVerticalBox(SpacerTop);

	if (!Header.IsEmpty())
	{
		USizeBox* HeaderSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		HeaderSize->SetHeightOverride(HeaderHeight);
		HeaderSize->SetMinDesiredHeight(HeaderHeight);

		UTextBlock* HeaderLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		HeaderLabel->SetText(Header);
		HeaderLabel->SetFont(ResolveSectionHeaderFont());
		HeaderLabel->SetJustification(ETextJustify::Left);
		HeaderLabel->SetColorAndOpacity(FSlateColor(CachedStyle.SectionHeaderText));
		HeaderSize->SetContent(HeaderLabel);

		if (UVerticalBoxSlot* HeaderSlot = Column->AddChildToVerticalBox(HeaderSize))
		{
			HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
			HeaderSlot->SetPadding(FMargin(CachedStyle.RowHorizontalPadding, 0.f));
		}
	}

	USizeBox* LineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	LineSize->SetHeightOverride(SeparatorHeight);
	LineSize->SetMinDesiredHeight(SeparatorHeight);

	UImage* Line = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Line->SetBrush(MakeDiveFlatColorBrush(CachedStyle.SectionSeparator));
	LineSize->SetContent(Line);

	if (UVerticalBoxSlot* LineSlot = Column->AddChildToVerticalBox(LineSize))
	{
		LineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		LineSlot->SetHorizontalAlignment(HAlign_Fill);
		LineSlot->SetPadding(FMargin(CachedStyle.RowHorizontalPadding, 0.f));
	}

	USizeBox* SpacerBottom = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	SpacerBottom->SetHeightOverride(SectionSpacing);
	SpacerBottom->SetMinDesiredHeight(SectionSpacing);
	Column->AddChildToVerticalBox(SpacerBottom);

	DividerSize->SetContent(Column);

	if (UVerticalBoxSlot* RowSlot = EntryList->AddChildToVerticalBox(DividerSize))
	{
		RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
	}
}

void UDIVEContextMenuWidget::AddActionRow(
	const FDIVEContextMenuEntry& Entry,
	const FSlateFontInfo& RowFont,
	const FButtonStyle& RowButtonStyle)
{
	if (!EntryList || !WidgetTree)
	{
		return;
	}

	USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	const float ViewportScale = GetViewportScaleSafe(this);
	const float RowHeight = SnapSlateUnitsToPhysicalPixel(CachedStyle.RowHeight, ViewportScale);
	RowSize->SetHeightOverride(RowHeight);
	RowSize->SetMinDesiredHeight(RowHeight);
	RowSize->SetWidthOverride(CachedStyle.MenuWidth);

	UDIVEContextMenuActionButton* RowButton =
		WidgetTree->ConstructWidget<UDIVEContextMenuActionButton>(UDIVEContextMenuActionButton::StaticClass());
	RowButton->SetStyle(RowButtonStyle);
	RowButton->SetIsEnabled(Entry.bEnabled);
	RowButton->Action = Entry.Action;
	RowButton->TargetKey = Entry.TargetKey;
	RowButton->OwnerWidget = this;

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(FormatRowLabel(Entry.DisplayName, Entry.bChecked));
	Label->SetFont(RowFont);
	Label->SetJustification(ETextJustify::Left);
	Label->SetColorAndOpacity(FSlateColor(
		Entry.bEnabled ? CachedStyle.RowText : CachedStyle.RowDisabledText));
	RowButton->SetContent(Label);
	RowSize->SetContent(RowButton);

	if (Entry.bEnabled)
	{
		RowButton->OnClicked.AddDynamic(RowButton, &UDIVEContextMenuActionButton::HandleClicked);
	}

	if (UVerticalBoxSlot* RowSlot = EntryList->AddChildToVerticalBox(RowSize))
	{
		RowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		RowSlot->SetHorizontalAlignment(HAlign_Fill);
	}
}

void UDIVEContextMenuWidget::RebuildList()
{
	if (!EntryList || !PanelBorder || !OuterFrame || !WidgetTree)
	{
		return;
	}

	OuterFrame->SetPadding(FMargin(1.f));
	OuterFrame->SetBrushColor(CachedStyle.PanelBorder);

	PanelBorder->SetPadding(FMargin(CachedStyle.PanelPadding));
	PanelBorder->SetBrushColor(CachedStyle.PanelBackground);

	EntryList->ClearChildren();

	const FSlateFontInfo RowFont = ResolveRowFont();
	const FButtonStyle RowButtonStyle = MakeRowButtonStyle(CachedStyle);

	for (const FDIVEContextMenuEntry& Entry : CachedEntries)
	{
		if (Entry.bIsSeparator)
		{
			AddSectionDivider(Entry.DisplayName);
		}
		else
		{
			AddActionRow(Entry, RowFont, RowButtonStyle);
		}
	}

	if (PanelSize)
	{
		PanelSize->SetWidthOverride(CachedStyle.MenuWidth);
	}

	UpdateDismissCaptureSize();
	SetScreenPosition(CachedScreenPosition);
}
