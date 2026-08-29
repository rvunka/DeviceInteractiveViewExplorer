// Copyright (c) 2026. All Rights Reserved.

#include "UI/DIVEValueReadoutWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PrimitiveComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "UI/SharedUmgStyle.h"
#include "Widgets/Layout/Anchors.h"

namespace
{
constexpr float ChipMarginSlate = 8.f;
const FVector2D ChipWorldOffsetSlate(18.f, -14.f);
const FVector2D ChipCursorOffsetSlate(16.f, 18.f);

float ViewportScaleSafe(const UObject* WorldContext)
{
	const float Scale = WorldContext ? UWidgetLayoutLibrary::GetViewportScale(WorldContext) : 1.f;
	return Scale > KINDA_SMALL_NUMBER ? Scale : 1.f;
}
}

UDIVEValueReadoutWidget::UDIVEValueReadoutWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDIVEValueReadoutWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RebuildChip();
}

void UDIVEValueReadoutWidget::TickAnchor()
{
	if (GetVisibility() != ESlateVisibility::Collapsed)
	{
		UpdateChipPosition();
	}
}

void UDIVEValueReadoutWidget::RebuildChip()
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->RootWidget = nullptr;
	RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
	RootCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	WidgetTree->RootWidget = RootCanvas;

	ChipBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChipBorder"));
	ChipBorder->SetPadding(FMargin(8.f, 4.f));
	ChipBorder->SetBrush(SharedUmgStyle::MakeFlatColorBrush(
		FLinearColor(0.07f, 0.08f, 0.09f, 0.92f),
		ESlateBrushDrawType::Image));

	UBorder* Outline = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChipOutline"));
	Outline->SetPadding(FMargin(1.f));
	Outline->SetBrush(SharedUmgStyle::MakeFlatColorBrush(
		FLinearColor(0.22f, 0.24f, 0.26f, 0.95f),
		ESlateBrushDrawType::Image));
	Outline->SetContent(ChipBorder);

	LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LabelText"));
	LabelText->SetFont(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 11));
	LabelText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.88f, 0.90f, 1.f)));
	ChipBorder->SetContent(LabelText);

	ChipSlot = RootCanvas->AddChildToCanvas(Outline);
	if (ChipSlot)
	{
		ChipSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		ChipSlot->SetAlignment(FVector2D(0.f, 1.f));
		ChipSlot->SetAutoSize(true);
		ChipSlot->SetPosition(FVector2D::ZeroVector);
	}
}

void UDIVEValueReadoutWidget::SetReadout(FText Label, const FDIVEInteractionValue& Value)
{
	if (LabelText)
	{
		LabelText->SetText(DIVE::FormatInteractionValueReadout(Label, Value));
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	UpdateChipPosition();
}

void UDIVEValueReadoutWidget::SetWorldAnchor(UPrimitiveComponent* Primitive)
{
	WorldAnchor = Primitive;
	UpdateChipPosition();
}

void UDIVEValueReadoutWidget::ClearReadout()
{
	WorldAnchor.Reset();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDIVEValueReadoutWidget::UpdateChipPosition()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		return;
	}

	FVector2D Slate = FVector2D::ZeroVector;
	bool bHaveSlate = false;

	if (const UPrimitiveComponent* Primitive = WorldAnchor.Get())
	{
		FVector2D WidgetPosition = FVector2D::ZeroVector;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
				PlayerController,
				Primitive->Bounds.Origin,
				WidgetPosition,
				true))
		{
			Slate = WidgetPosition + ChipWorldOffsetSlate;
			bHaveSlate = true;
		}
	}

	if (!bHaveSlate)
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		if (PlayerController->GetMousePosition(MouseX, MouseY))
		{
			Slate = FVector2D(MouseX, MouseY) / ViewportScaleSafe(this) + ChipCursorOffsetSlate;
			bHaveSlate = true;
		}
	}

	if (bHaveSlate)
	{
		SetChipSlatePosition(Slate);
	}
}

void UDIVEValueReadoutWidget::SetChipSlatePosition(FVector2D SlatePosition)
{
	if (!ChipSlot)
	{
		return;
	}

	const FVector2D ViewportSlate = UWidgetLayoutLibrary::GetViewportSize(this) / ViewportScaleSafe(this);
	SlatePosition.X = FMath::Clamp(SlatePosition.X, ChipMarginSlate, FMath::Max(ChipMarginSlate, ViewportSlate.X - ChipMarginSlate));
	SlatePosition.Y = FMath::Clamp(SlatePosition.Y, ChipMarginSlate, FMath::Max(ChipMarginSlate, ViewportSlate.Y - ChipMarginSlate));
	ChipSlot->SetPosition(SlatePosition);
}
