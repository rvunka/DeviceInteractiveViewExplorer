// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVEDeviceAction.h"
#include "DIVETypes.h"
#include "UI/DIVEContextMenuStyle.h"

#include "DIVEContextMenuUIComponent.generated.h"

class UDIVEContextMenuWidget;
class UDIVEValueReadoutWidget;

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Context Menu UI"))
class DIVERUNTIME_API UDIVEContextMenuUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIVEContextMenuUIComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, Category = "DIVE|ContextMenu")
	int32 ViewportZOrder = 20;

	UPROPERTY(EditAnywhere, Category = "DIVE|ContextMenu")
	FDIVEContextMenuStyle MenuStyle;

	/** Optional Blueprint subclass for the context menu widget. Empty = UDIVEContextMenuWidget. */
	UPROPERTY(EditAnywhere, Category = "DIVE|ContextMenu", meta = (
		DisplayName = "Context Menu Widget Class"))
	TSubclassOf<UDIVEContextMenuWidget> ContextMenuWidgetClass;

	/** Optional override for continuous-action value HUD. Empty = UDIVEValueReadoutWidget. */
	UPROPERTY(EditAnywhere, Category = "DIVE|Action|HUD", meta = (
		DisplayName = "Value Readout Widget Class"))
	TSubclassOf<UDIVEValueReadoutWidget> ValueReadoutWidgetClass;

	UPROPERTY(EditAnywhere, Category = "DIVE|Action|HUD")
	int32 ValueReadoutZOrder = 15;

protected:
	UFUNCTION()
	void HandleContextMenuVisibilityChanged(bool bIsOpen);

	UFUNCTION()
	void HandleSessionEnded(EDIVESessionEndReason Reason, AActor* DeviceHost);

	UFUNCTION()
	void HandleContextMenuEntrySelected(UDIVEDeviceAction* Action, FName TargetKey, FName BindingId);

	UFUNCTION()
	void HandleContextMenuDismissed();

	UFUNCTION()
	void HandleInteractionValueChanged(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		float NormalizedValue);

	void BindSessionDelegates();
	void UnbindSessionDelegates();
	void ShowContextMenu();
	void HideContextMenu();
	void EnsureValueReadoutWidget();
	void HideValueReadout();
	bool IsLocallyControlledOwner() const;

	UPROPERTY(Transient)
	TObjectPtr<UDIVEContextMenuWidget> ContextMenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UDIVEValueReadoutWidget> ValueReadoutWidget;
};
