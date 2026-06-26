// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Components/ActorComponent.h"
#include "DIVETypes.h"

#include "DIVEOperationsUIComponent.generated.h"

class UDIVEOperationsListWidget;

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Operations UI"))
class DIVERUNTIME_API UDIVEOperationsUIComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIVEOperationsUIComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "DIVE|Operations")
	void CycleSelection(int32 Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Operations")
	void HandleExecutePressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|Operations")
	void HandleExecuteReleased();

	UPROPERTY(EditAnywhere, Category = "DIVE|Operations")
	float DefaultHoldDuration = 0.45f;

	UPROPERTY(EditAnywhere, Category = "DIVE|Operations")
	int32 ViewportZOrder = 10;

protected:
	UFUNCTION()
	void HandleSessionStarted(AActor* DeviceHost, class UDIVEInspectableComponent* Inspectable);

	UFUNCTION()
	void HandleSessionEnded(EDIVESessionEndReason Reason, AActor* DeviceHost);

	UFUNCTION()
	void HandleFocusChanged(const FDIVEFocusTarget& FocusTarget);

	void BindSessionDelegates();
	void UnbindSessionDelegates();
	void RefreshOperations();
	void ResetHoldState();
	const FDIVEOperationDescriptor* GetSelectedOperation() const;
	float ResolveHoldDuration(const FDIVEOperationDescriptor& Operation) const;
	bool IsLocallyControlledOwner() const;

	UPROPERTY(Transient)
	TObjectPtr<UDIVEOperationsListWidget> OperationsWidget;

	TArray<FDIVEOperationDescriptor> AvailableOperations;
	int32 SelectedIndex = INDEX_NONE;

	bool bHoldInProgress = false;
	float HoldElapsed = 0.f;
	FName HoldOperationId = NAME_None;
};
