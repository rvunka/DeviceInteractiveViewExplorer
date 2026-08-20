// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "DIVEDeviceAction.h"

#include "DIVEValueReadoutWidget.generated.h"

/** On-screen value readout. Formats FDIVEInteractionValue; host may replace via ValueReadoutWidgetClass. */
UCLASS()
class DIVERUNTIME_API UDIVEValueReadoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDIVEValueReadoutWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "DIVE|UI")
	void SetReadout(FText Label, const FDIVEInteractionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "DIVE|UI")
	void SetNormalizedReadout(FText Label, float NormalizedValue);

	UFUNCTION(BlueprintCallable, Category = "DIVE|UI")
	void ClearReadout();

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> LabelText;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> PanelBorder;
};
