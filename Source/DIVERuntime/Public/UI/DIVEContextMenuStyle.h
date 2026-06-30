// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVEContextMenuStyle.generated.h"

UENUM(BlueprintType)
enum class EDIVEContextMenuTypeface : uint8
{
	Regular UMETA(DisplayName = "Regular"),
	Bold UMETA(DisplayName = "Bold"),
	Light UMETA(DisplayName = "Light")
};

USTRUCT(BlueprintType)
struct DIVERUNTIME_API FDIVEContextMenuStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Panel")
	FLinearColor PanelBackground = FLinearColor(0.025f, 0.045f, 0.075f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Panel")
	FLinearColor PanelBorder = FLinearColor(0.18f, 0.26f, 0.34f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Panel", meta = (ClampMin = "0", ClampMax = "16"))
	float PanelPadding = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Panel", meta = (ClampMin = "96", ClampMax = "480"))
	float MenuWidth = 184.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Rows")
	FLinearColor RowHoverBackground = FLinearColor(0.08f, 0.14f, 0.24f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Rows")
	FLinearColor RowPressedBackground = FLinearColor(0.06f, 0.11f, 0.20f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Row Text")
	FLinearColor RowText = FLinearColor(0.82f, 0.86f, 0.90f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Row Text")
	FLinearColor RowDisabledText = FLinearColor(0.42f, 0.44f, 0.47f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Row Text", meta = (ClampMin = "6", ClampMax = "24"))
	int32 RowFontSize = 13;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Row Text")
	EDIVEContextMenuTypeface RowTypeface = EDIVEContextMenuTypeface::Regular;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Row Text", meta = (AllowedClasses = "/Script/Engine.Font"))
	TObjectPtr<UObject> RowFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Layout", meta = (ClampMin = "0", ClampMax = "20"))
	float RowHorizontalPadding = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Layout", meta = (ClampMin = "20", ClampMax = "48"))
	float RowHeight = 26.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Sections")
	FLinearColor SectionSeparator = FLinearColor(0.45f, 0.56f, 0.68f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu|Sections", meta = (ClampMin = "0", ClampMax = "12"))
	float SectionSpacing = 4.f;
};
