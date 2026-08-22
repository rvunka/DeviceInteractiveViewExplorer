// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "DIVEDeviceAction.h"

#include "DIVEValueReadoutWidget.generated.h"

class UPrimitiveComponent;

/** Compact value chip next to the driven primitive (cursor fallback). */
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
	void SetWorldAnchor(UPrimitiveComponent* Primitive);

	UFUNCTION(BlueprintCallable, Category = "DIVE|UI")
	void ClearReadout();

	void TickAnchor();

protected:
	virtual void NativeOnInitialized() override;

	void RebuildChip();
	void UpdateChipPosition();
	void SetChipSlatePosition(FVector2D SlatePosition);

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> ChipBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> LabelText;

	UPROPERTY(Transient)
	TWeakObjectPtr<UPrimitiveComponent> WorldAnchor;

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanelSlot> ChipSlot;
};
