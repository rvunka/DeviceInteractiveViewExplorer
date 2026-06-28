// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DIVEConvention.h"
#include "DIVETypes.h"
#include "Engine/EngineTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include "DIVEInspectableComponent.generated.h"

class UDIVEDeviceDefinitionAsset;
class UDIVEAnchorComponent;
class UPrimitiveComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVESessionLifecycle, bool, bSessionActive);

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent))
class DIVERUNTIME_API UDIVEInspectableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDIVEInspectableComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE")
	TObjectPtr<UDIVEDeviceDefinitionAsset> DeviceDefinition;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick")
	FName SkipComponentTag = TEXT("DIVE.Skip");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (ClampMin = "0.0"))
	float MinPickBoundsRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick")
	TEnumAsByte<ECollisionChannel> PickTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|View", meta = (
		DisplayName = "Default Start Focus Id",
		ToolTip = "Anchor PartId or pickable mesh component name to focus when the session starts."))
	FName DefaultStartFocusId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	bool bUseDeviceDefinitionSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.01", EditCondition = "!bUseDeviceDefinitionSettings"))
	float OrbitSensitivity = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.01", EditCondition = "!bUseDeviceDefinitionSettings"))
	float ZoomSensitivity = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|View", meta = (ClampMin = "50.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float FocusBlendDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Presentation")
	EDIVEWorldDimPolicy WorldDimPolicy = EDIVEWorldDimPolicy::None;

	UPROPERTY(BlueprintAssignable, Category = "DIVE")
	FOnDIVESessionLifecycle OnSessionLifecycle;

	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (DisplayName = "Request Session"))
	bool RequestSession();

	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (DisplayName = "Request Session (With Params)"))
	bool RequestSessionWithParams(const FDIVESessionParams& Params);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void BuildSemanticRegistry();

	UFUNCTION(BlueprintPure, Category = "DIVE")
	const FDIVEPartTree& GetSemanticRegistry() const { return SemanticRegistry; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsSessionActive() const { return bSessionActive; }

	void UpdateAnchorSessionPresentation(const FDIVEFocusTarget& FocusedTarget);

	UFUNCTION(BlueprintPure, Category = "DIVE")
	FName ResolveSemanticPartId(const UPrimitiveComponent* Primitive) const;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsPrimitivePickable(const UPrimitiveComponent* Primitive) const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveOrbitSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveZoomSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|View")
	float GetEffectiveDefaultOrbitDistance() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Pick")
	TEnumAsByte<ECollisionChannel> GetEffectivePickTraceChannel() const { return PickTraceChannel; }

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveFocusBlendDuration() const { return FocusBlendDuration; }

	UFUNCTION(BlueprintPure, Category = "DIVE|View")
	bool TryResolveStartFocusTarget(FName FocusObjectId, FDIVEFocusTarget& OutTarget) const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Presentation")
	EDIVEWorldDimPolicy GetEffectiveWorldDimPolicy() const { return WorldDimPolicy; }

	UFUNCTION(BlueprintNativeEvent, Category = "DIVE|ContextMenu")
	void AppendContextMenuEntries(const FDIVEFocusTarget& PickTarget, TArray<FDIVEContextMenuEntry>& InOutEntries);

	UFUNCTION(BlueprintNativeEvent, Category = "DIVE|ContextMenu")
	bool ExecuteContextMenuAction(FName ActionId, const FDIVEFocusTarget& PickTarget);

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool FindAnchorNode(FName PartId, FDIVEPartNode& OutNode) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
	UPROPERTY(VisibleAnywhere, Category = "DIVE")
	FDIVEPartTree SemanticRegistry;

	UPROPERTY(VisibleAnywhere, Category = "DIVE")
	bool bSessionActive = false;

	friend class UDIVESessionSubsystem;

	void NotifySessionLifecycle(bool bActive);
};
