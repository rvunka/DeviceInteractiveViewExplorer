// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DIVEActionCatalogAsset.h"
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
class UMaterialInterface;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (
		DisplayName = "Pick Proxy Component Tag",
		ToolTip = "Tag for Hidden-in-Game meshes used as hit volumes. Shape components need no tag."))
	FName PickProxyComponentTag = DIVE::kPickProxyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (ClampMin = "0.0"))
	float MinPickBoundsRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (
		DisplayName = "Pick Trace Channel",
		ToolTip = "Screen-pick channel (default Visibility). Block that response on Box/Sphere volumes."))
	TEnumAsByte<ECollisionChannel> PickTraceChannel = ECC_Visibility;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (
		DisplayName = "Pick Interaction Exclusions",
		ToolTip = "Component name or PartId — no menu, primary action, or hover."))
	TArray<FName> PickInteractionExclusions;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick|Hover", meta = (
		DisplayName = "Hover Overlay Material",
		ToolTip = "Default overlay for interactive meshes under the cursor."))
	TSoftObjectPtr<UMaterialInterface> DefaultPickHoverOverlayMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick|Hover", meta = (
		DisplayName = "Pick Hover Overlay By Component",
		ToolTip = "Component name or PartId → overlay override."))
	TMap<FName, TSoftObjectPtr<UMaterialInterface>> PickHoverOverlayByComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		EditCondition = "!bUseDeviceDefinitionSettings",
		ToolTip = "Scale zoom step with orbit distance (finer near, coarser far)."))
	bool bScaleZoomWithOrbitDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ClampMin = "1.0",
		EditCondition = "!bUseDeviceDefinitionSettings && bScaleZoomWithOrbitDistance",
		ToolTip = "Orbit distance (cm) where Zoom Sensitivity maps 1:1."))
	float ZoomDistanceReferenceCm = DIVE::kDefaultZoomDistanceReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float MinOrbitDistanceCm = DIVE::kDefaultMinOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float MaxOrbitDistanceCm = DIVE::kDefaultMaxOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ClampMin = "1.0",
		EditCondition = "!bUseDeviceDefinitionSettings",
		ToolTip = "Focus orbit distance ≈ SphereRadius × this."))
	float FocusOrbitFitMultiplier = DIVE::kDefaultFocusOrbitFitMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ClampMin = "0.0",
		EditCondition = "!bUseDeviceDefinitionSettings",
		ToolTip = "Near zoom floor ≈ SphereRadius × this while focused."))
	float FocusNearPaddingFactor = DIVE::kDefaultFocusNearPaddingFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|View", meta = (ClampMin = "1.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float FocusBlendDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Actions", meta = (
		DisplayName = "Action Catalog",
		ToolTip = "Shared Data Asset bindings/sections; merged with component Bindings."))
	TObjectPtr<UDIVEActionCatalogAsset> ActionCatalog;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Actions", meta = (
		DisplayName = "Bindings",
		ToolTip = "Local bindings (defaults: Focus/Isolate/Admin). Clear for none."))
	TArray<FDIVEActionBinding> Bindings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Actions", meta = (
		DisplayName = "Sections",
		ToolTip = "Menu sections for local Bindings (Standard / Admin by default)."))
	TArray<FDIVEMenuSection> Sections;

	UPROPERTY(BlueprintAssignable, Category = "DIVE")
	FOnDIVESessionLifecycle OnSessionLifecycle;

	/**
	 * Fired by the session after a successful instant Execute, or after a successful continuous Begin.
	 * Bind from the device actor (Details +). Branch with Cast To your action BP class (no Switch-on-class in BP).
	 */
	UPROPERTY(BlueprintAssignable, Category = "DIVE|Actions")
	FOnDIVEActionExecuted OnActionExecuted;

	/** Runtime fan-out used by the session after a successful action start. */
	void NotifyActionExecuted(UDIVEDeviceAction* Action, const FDIVEActionContext& Context);

	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (DisplayName = "Request Session"))
	bool RequestSession();

	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (DisplayName = "Request Session (With Params)"))
	bool RequestSessionWithParams(const FDIVESessionParams& Params);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void BuildSemanticRegistry();

	/**
	 * Returns available SectionId values for the GetOptions dropdown on Binding.SectionId.
	 * Includes built-in IDs (Standard, Admin) and all authored section IDs from this component and
	 * the linked Action Catalog.
	 */
	UFUNCTION()
	TArray<FName> GetAvailableSectionIds() const;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	const FDIVEPartTree& GetSemanticRegistry() const { return SemanticRegistry; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsSessionActive() const { return bSessionActive; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	FName ResolveSemanticPartId(const UPrimitiveComponent* Primitive) const;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsPrimitivePickable(const UPrimitiveComponent* Primitive) const;

	/** True for Box/Sphere/Capsule or components tagged Pick Proxy Component Tag. */
	UFUNCTION(BlueprintPure, Category = "DIVE|Pick")
	bool IsPickProxyPrimitive(const UPrimitiveComponent* Primitive) const;

	/** Pickable and not listed in Pick Interaction Exclusions. */
	UFUNCTION(BlueprintPure, Category = "DIVE|Pick")
	bool IsPrimitiveInteractive(const UPrimitiveComponent* Primitive) const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveOrbitSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveZoomSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	bool GetEffectiveScaleZoomWithOrbitDistance() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveZoomDistanceReferenceCm() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveMinOrbitDistanceCm() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveMaxOrbitDistanceCm() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveFocusOrbitFitMultiplier() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveFocusNearPaddingFactor() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveDefaultOrbitDistance() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	FDIVECameraEffectiveSettings GetEffectiveCameraSettings() const;

	/** Orbit distance used when focusing a target (primitive fit or device default). */
	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float ComputeOrbitDistanceForFocus(const FDIVEFocusTarget& Target) const;

	/** Near-zoom clearance radius while this target is focused (0 when not a primitive). */
	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float ComputeFocusClearanceRadius(const FDIVEFocusTarget& Target) const;

	UFUNCTION(BlueprintPure, Category = "DIVE|View")
	bool TryResolveStartFocusTarget(FName FocusObjectId, FDIVEFocusTarget& OutTarget) const;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool FindAnchorNode(FName PartId, FDIVEPartNode& OutNode) const;

	void GatherAuthoredBindings(TArray<const FDIVEActionBinding*>& OutBindings) const;

	void GatherAuthoredSections(TArray<FDIVEMenuSection>& OutSections) const;

	bool DoesTargetQueryMatchPick(const FDIVETargetQuery& Query, const FDIVEFocusTarget& PickTarget) const;

	FName ResolveTargetKeyForQuery(const FDIVETargetQuery& Query, const FDIVEFocusTarget& PickTarget) const;

	void GatherMatchingBindings(
		const FDIVEFocusTarget& PickTarget,
		TArray<const FDIVEActionBinding*>& OutBindings) const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Actions")
	TArray<UDIVEDeviceAction*> GetActionInstances(FName BindingId) const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Actions", meta = (
		DeterminesOutputType = "ActionClass"))
	UDIVEDeviceAction* FindActionInstance(
		TSubclassOf<UDIVEDeviceAction> ActionClass,
		FName BindingId = NAME_None) const;

	bool TryResolvePrimaryAction(
		const FDIVEFocusTarget& PickTarget,
		UDIVEDeviceAction*& OutAction,
		FName& OutTargetKey) const;

	void AppendConfiguredContextMenuEntries(
		const FDIVEFocusTarget& PickTarget,
		TArray<FDIVEContextMenuEntry>& InOutEntries) const;

	UMaterialInterface* ResolvePickHoverOverlayMaterial(const FDIVEFocusTarget& PickTarget) const;

	FDIVEActionContext MakeActionContext(
		const FDIVEFocusTarget& PickTarget,
		FName TargetKey,
		const FVector2D& ScreenPosition = FVector2D::ZeroVector,
		const FHitResult& PickHit = FHitResult()) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

	virtual void PostInitProperties() override;

private:
	UPROPERTY(Transient)
	FDIVEPartTree SemanticRegistry;

	UPROPERTY(Transient)
	bool bSessionActive = false;

	friend class UDIVESessionSubsystem;

	void NotifySessionLifecycle(bool bActive);
	void SeedDefaultBindingsIfNeeded();

	bool MatchesComponentNameValue(const UPrimitiveComponent* Primitive, FName MatchValue) const;
	bool IsPrimitiveExcludedFromPickInteraction(const UPrimitiveComponent* Primitive) const;
	bool FindPickHoverOverlaySoftMaterial(const FDIVEFocusTarget& PickTarget, TSoftObjectPtr<UMaterialInterface>& OutSoftMaterial) const;
};
