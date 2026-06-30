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
#include "Engine/GameViewportClient.h"
#include "InputCoreTypes.h"
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
	ButtonStyle.Normal = MakeFlatColorBrush(FLinearColor::Transparent);
	ButtonStyle.Hovered = MakeFlatColorBrush(Style.RowHoverBackground);
	ButtonStyle.Pressed = MakeFlatColorBrush(Style.RowPressedBackground);
	ButtonStyle.Disabled = MakeFlatColorBrush(FLinearColor::Transparent);
	ButtonStyle.NormalPadding = FMargin(Style.RowHorizontalPadding, 0.f);
	ButtonStyle.PressedPadding = FMargin(Style.RowHorizontalPadding, 0.f);
	return ButtonStyle;
}

FButtonStyle MakeTransparentDismissButtonStyle()
{
	FButtonStyle ButtonStyle;
	const FSlateBrush TransparentBrush = MakeFlatColorBrush(FLinearColor::Transparent);
	ButtonStyle.Normal = TransparentBrush;
	ButtonStyle.Hovered = TransparentBrush;
	ButtonStyle.Pressed = TransparentBrush;
	ButtonStyle.Disabled = TransparentBrush;
	ButtonStyle.NormalPadding = FMargin(0.f);
	ButtonStyle.PressedPadding = FMargin(0.f);
	return ButtonStyle;
}

FSlateFontInfo ResolveMenuFont(const UObject* FontObject, EDIVEContextMenuTypeface Typeface, int32 Size)
{
	const int32 ClampedSize = FMath::Clamp(Size, 6, 24);
	FName TypefaceName = NAME_None;

	switch (Typeface)
	{
	case EDIVEContextMenuTypeface::Bold:
		TypefaceName = FName(TEXT("Bold"));
		break;
	case EDIVEContextMenuTypeface::Light:
		TypefaceName = FName(TEXT("Light"));
		break;
	default:
		TypefaceName = FName(TEXT("Regular"));
		break;
	}

	if (FontObject)
	{
		return FSlateFontInfo(FontObject, ClampedSize, TypefaceName);
	}

	return FCoreStyle::GetDefaultFontStyle(*TypefaceName.ToString(), ClampedSize);
}
} // namespace

UDIVEContextMenuWidget::UDIVEContextMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UDIVEContextMenuRowHost::HandleClicked()
{
	if (UDIVEContextMenuWidget* Menu = OwnerWidget.Get())
	{
		Menu->HandleEntryClicked(ActionId);
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
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OuterFrame)
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

float UDIVEContextMenuWidget::GetEstimatedMenuHeight() const
{
	const float ViewportScale = GetViewportScaleSafe(this);
	const float RowHeight = SnapSlateUnitsToPhysicalPixel(CachedStyle.RowHeight, ViewportScale);
	const float SectionSpacing = SnapSlateUnitsToPhysicalPixel(CachedStyle.SectionSpacing, ViewportScale);
	const float SeparatorHeight = OnePhysicalPixelInSlateUnits(ViewportScale);

	float Height = SnapSlateUnitsToPhysicalPixel(CachedStyle.PanelPadding * 2.f + 2.f, ViewportScale);
	bool bNextRowStartsSection = false;

	for (const FDIVEContextMenuEntry& Entry : CachedEntries)
	{
		if (Entry.bIsSeparator)
		{
			bNextRowStartsSection = true;
		}
		else
		{
			if (bNextRowStartsSection)
			{
				Height += SectionSpacing * 2.f + SeparatorHeight;
				bNextRowStartsSection = false;
			}

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
	return ResolveMenuFont(CachedStyle.RowFont, CachedStyle.RowTypeface, CachedStyle.RowFontSize);
}

void UDIVEContextMenuWidget::HandleEntryClicked(FName ActionId)
{
	if (!ActionId.IsNone())
	{
		OnEntrySelected.Broadcast(ActionId);
	}
}

void UDIVEContextMenuWidget::AddActionRow(
	const FDIVEContextMenuEntry& Entry,
	const FSlateFontInfo& RowFont,
	const FButtonStyle& RowButtonStyle,
	bool bStartsSection)
{
	if (!EntryList || !WidgetTree)
	{
		return;
	}

	USizeBox* RowSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	const float ViewportScale = GetViewportScaleSafe(this);
	const float RowHeight = SnapSlateUnitsToPhysicalPixel(CachedStyle.RowHeight, ViewportScale);
	const float SectionSpacing = SnapSlateUnitsToPhysicalPixel(CachedStyle.SectionSpacing, ViewportScale);
	const float SeparatorHeight = OnePhysicalPixelInSlateUnits(ViewportScale);
	const float SectionHeaderHeight = bStartsSection
		? SectionSpacing * 2.f + SeparatorHeight
		: 0.f;
	const float TotalRowHeight = RowHeight + SectionHeaderHeight;
	RowSize->SetHeightOverride(TotalRowHeight);
	RowSize->SetMinDesiredHeight(TotalRowHeight);
	RowSize->SetWidthOverride(CachedStyle.MenuWidth);

	UButton* RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
	RowButton->SetStyle(RowButtonStyle);
	RowButton->SetIsEnabled(Entry.bEnabled);

	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Label->SetText(Entry.DisplayName);
	Label->SetFont(RowFont);
	Label->SetJustification(ETextJustify::Left);
	Label->SetColorAndOpacity(FSlateColor(
		Entry.bEnabled ? CachedStyle.RowText : CachedStyle.RowDisabledText));
	RowButton->SetContent(Label);

	if (bStartsSection)
	{
		UVerticalBox* SectionColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		USizeBox* SpacerTop = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SpacerTop->SetHeightOverride(SectionSpacing);
		SpacerTop->SetMinDesiredHeight(SectionSpacing);
		SectionColumn->AddChildToVerticalBox(SpacerTop);

		USizeBox* LineSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		LineSize->SetHeightOverride(SeparatorHeight);
		LineSize->SetMinDesiredHeight(SeparatorHeight);

		UImage* Line = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Line->SetBrush(MakeFlatColorBrush(CachedStyle.SectionSeparator));
		LineSize->SetContent(Line);

		if (UVerticalBoxSlot* LineSlot = SectionColumn->AddChildToVerticalBox(LineSize))
		{
			LineSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			LineSlot->SetHorizontalAlignment(HAlign_Fill);
			LineSlot->SetPadding(FMargin(CachedStyle.RowHorizontalPadding, 0.f));
		}

		USizeBox* SpacerBottom = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SpacerBottom->SetHeightOverride(SectionSpacing);
		SpacerBottom->SetMinDesiredHeight(SectionSpacing);
		SectionColumn->AddChildToVerticalBox(SpacerBottom);

		USizeBox* ActionSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		ActionSize->SetHeightOverride(RowHeight);
		ActionSize->SetMinDesiredHeight(RowHeight);
		ActionSize->SetContent(RowButton);
		SectionColumn->AddChildToVerticalBox(ActionSize);

		RowSize->SetContent(SectionColumn);
	}
	else
	{
		RowSize->SetContent(RowButton);
	}

	if (Entry.bEnabled)
	{
		UDIVEContextMenuRowHost* RowHost = NewObject<UDIVEContextMenuRowHost>(
			this,
			MakeUniqueObjectName(this, UDIVEContextMenuRowHost::StaticClass(), TEXT("ContextMenuRowHost")));
		RowHost->ActionId = Entry.ActionId;
		RowHost->OwnerWidget = this;
		RowHosts.Add(RowHost);
		RowButton->OnClicked.AddDynamic(RowHost, &UDIVEContextMenuRowHost::HandleClicked);
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
	RowHosts.Reset();

	const FSlateFontInfo RowFont = ResolveRowFont();
	const FButtonStyle RowButtonStyle = MakeRowButtonStyle(CachedStyle);
	bool bNextRowStartsSection = false;

	for (const FDIVEContextMenuEntry& Entry : CachedEntries)
	{
		if (Entry.bIsSeparator)
		{
			bNextRowStartsSection = true;
		}
		else
		{
			AddActionRow(Entry, RowFont, RowButtonStyle, bNextRowStartsSection);
			bNextRowStartsSection = false;
		}
	}

	if (PanelSize)
	{
		PanelSize->SetWidthOverride(CachedStyle.MenuWidth);
	}

	UpdateDismissCaptureSize();
	SetScreenPosition(CachedScreenPosition);
}
