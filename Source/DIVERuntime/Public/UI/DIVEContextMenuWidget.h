// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "DIVEDeviceAction.h"
#include "UI/DIVEContextMenuStyle.h"

#include "DIVEContextMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDIVEContextMenuEntrySelected,
	UDIVEDeviceAction*,
	Action,
	FName,
	TargetKey,
	FName,
	BindingId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDIVEContextMenuDismissed);

class UDIVEContextMenuWidget;

UCLASS()
class UDIVEContextMenuActionButton : public UButton
{
	GENERATED_BODY()

public:
	TObjectPtr<UDIVEDeviceAction> Action = nullptr;
	FName TargetKey = NAME_None;
	FName BindingId = NAME_None;
	TWeakObjectPtr<UDIVEContextMenuWidget> OwnerWidget;

	UFUNCTION()
	void HandleClicked();
};

UCLASS()
class DIVERUNTIME_API UDIVEContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UDIVEContextMenuWidget(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintAssignable, Category = "DIVE|ContextMenu")
	FOnDIVEContextMenuEntrySelected OnEntrySelected;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|ContextMenu")
	FOnDIVEContextMenuDismissed OnDismissed;

	void SetStyle(const FDIVEContextMenuStyle& InStyle);
	void SetEntries(const TArray<FDIVEContextMenuEntry>& Entries);
	void SetScreenPosition(const FVector2D& InScreenPosition);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	void RebuildList();
	void UpdateDismissCaptureSize();
	void ApplyMenuSlotPosition();
	void AddActionRow(
		const FDIVEContextMenuEntry& Entry,
		const FSlateFontInfo& RowFont,
		const FButtonStyle& RowButtonStyle);
	void AddSectionDivider(const FText& Header);
	FVector2D ClampPositionToViewport(const FVector2D& ScreenPosition) const;
	FSlateFontInfo ResolveRowFont() const;
	FSlateFontInfo ResolveSectionHeaderFont() const;
	float GetEstimatedMenuHeight() const;
	float GetSectionDividerHeight(const FText& Header) const;

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<class UButton> DismissCapture;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> OuterFrame;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> PanelBorder;

	UPROPERTY(Transient)
	TObjectPtr<class USizeBox> PanelSize;

	UPROPERTY(Transient)
	TObjectPtr<class UVerticalBox> EntryList;

	UPROPERTY(Transient)
	TArray<FDIVEContextMenuEntry> CachedEntries;

	FDIVEContextMenuStyle CachedStyle;
	/** Viewport pixels from GetMousePosition / pick — converted to slate only when applying the slot. */
	FVector2D CachedScreenPosition = FVector2D::ZeroVector;

	friend class UDIVEContextMenuActionButton;

	void HandleEntryClicked(UDIVEDeviceAction* Action, FName TargetKey, FName BindingId);

	UFUNCTION()
	void HandleDismissCaptureClicked();
};
