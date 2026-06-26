// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "DIVETypes.h"

#include "DIVEOperationsListWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class UVerticalBox;

UCLASS()
class DIVERUNTIME_API UDIVEOperationsListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOperations(const TArray<FDIVEOperationDescriptor>& Operations, int32 SelectedIndex, float HoldProgress);

	void SetHoldProgress(float HoldProgress);

protected:
	virtual void NativeOnInitialized() override;

	void RebuildList();

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RootList;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> OperationList;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> SelectedHoldProgress;

	UPROPERTY(Transient)
	TArray<FDIVEOperationDescriptor> CachedOperations;

	UPROPERTY(Transient)
	int32 CachedSelectedIndex = INDEX_NONE;

	UPROPERTY(Transient)
	float CachedHoldProgress = 0.f;
};
