// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "DIVETypes.h"
#include "UI/DIVESessionChromeStyle.h"

#include "DIVESessionChromeWidget.generated.h"

UCLASS()
class DIVERUNTIME_API UDIVESessionChromeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDIVESessionChromeWidget(const FObjectInitializer& ObjectInitializer);

	void SetStyle(const FDIVESessionChromeStyle& InStyle);
	void SetInteractionMode(EDIVESessionInteractionMode InMode);

protected:
	virtual void NativeOnInitialized() override;

	void RebuildChrome();
	FText ResolveModeLabel(EDIVESessionInteractionMode InMode) const;
	FLinearColor ResolveModeTextColor(EDIVESessionInteractionMode InMode) const;

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> ModePanel;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> ModeLabel;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> ModeHint;

	FDIVESessionChromeStyle CachedStyle;
	EDIVESessionInteractionMode CachedMode = EDIVESessionInteractionMode::Interact;
};
