// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "DIVETypes.h"

#include "DIVEContextMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVEContextMenuEntrySelected, FName, ActionId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDIVEContextMenuDismissed);

class UDIVEContextMenuWidget;

UCLASS()
class UDIVEContextMenuRowHost : public UObject
{
	GENERATED_BODY()

public:
	FName ActionId = NAME_None;
	TWeakObjectPtr<UDIVEContextMenuWidget> OwnerWidget;

	UFUNCTION()
	void HandleClicked();
};

UCLASS()
class DIVERUNTIME_API UDIVEContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "DIVE|ContextMenu")
	FOnDIVEContextMenuEntrySelected OnEntrySelected;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|ContextMenu")
	FOnDIVEContextMenuDismissed OnDismissed;

	void SetEntries(const TArray<FDIVEContextMenuEntry>& Entries);
	void SetScreenPosition(const FVector2D& InScreenPosition);

protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void RebuildList();
	FVector2D ClampPositionToViewport(const FVector2D& ScreenPosition) const;

	UPROPERTY(Transient)
	TObjectPtr<class UVerticalBox> EntryList;

	UPROPERTY(Transient)
	TArray<FDIVEContextMenuEntry> CachedEntries;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDIVEContextMenuRowHost>> RowHosts;

	FVector2D CachedScreenPosition = FVector2D::ZeroVector;

	friend class UDIVEContextMenuRowHost;

	void HandleEntryClicked(FName ActionId);
};
