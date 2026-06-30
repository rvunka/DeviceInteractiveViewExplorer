// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "DIVESessionChromeStyle.generated.h"

USTRUCT(BlueprintType)
struct DIVERUNTIME_API FDIVESessionChromeStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Panel")
	FLinearColor PanelBackground = FLinearColor(0.025f, 0.045f, 0.075f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Panel")
	FLinearColor PanelBorder = FLinearColor(0.18f, 0.26f, 0.34f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Panel", meta = (ClampMin = "0", ClampMax = "24"))
	float PanelPadding = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Panel", meta = (ClampMin = "8", ClampMax = "48"))
	float Margin = 16.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Text")
	FLinearColor ModeText = FLinearColor(0.95f, 0.88f, 0.35f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Text")
	FLinearColor DefaultModeText = FLinearColor(0.82f, 0.86f, 0.90f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Text")
	FLinearColor HintText = FLinearColor(0.55f, 0.58f, 0.62f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Text", meta = (ClampMin = "8", ClampMax = "24"))
	int32 ModeFontSize = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|SessionChrome|Text", meta = (ClampMin = "8", ClampMax = "20"))
	int32 HintFontSize = 11;
};
