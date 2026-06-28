// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVETypes.h"

#include "DIVEContextMenuUIComponent.generated.h"

class UDIVEContextMenuWidget;

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

protected:
	UFUNCTION()
	void HandleContextMenuVisibilityChanged(bool bIsOpen);

	UFUNCTION()
	void HandleSessionEnded(EDIVESessionEndReason Reason, AActor* DeviceHost);

	UFUNCTION()
	void HandleContextMenuEntrySelected(FName ActionId);

	UFUNCTION()
	void HandleContextMenuDismissed();

	void BindSessionDelegates();
	void UnbindSessionDelegates();
	void ShowContextMenu();
	void HideContextMenu();
	bool IsLocallyControlledOwner() const;

	UPROPERTY(Transient)
	TObjectPtr<UDIVEContextMenuWidget> ContextMenuWidget;
};
