// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DIVEActionCatalogAsset.h"
#include "DIVEActionExecution.h"
#include "DIVEConvention.h"
#include "DIVETypes.h"
#include "Engine/EngineTypes.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#include "UObject/UnrealType.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		EditCondition = "!bUseDeviceDefinitionSettings"))
	FDIVECameraSettings CameraSettings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Actions", meta = (
		DisplayName = "Action Catalog",
		ToolTip = "Template Data Asset. Runtime executes per-Inspectable copies (not the asset inners). Do not bind OnExecuted / OnValueChanged on the catalog template."))
	TObjectPtr<UDIVEActionCatalogAsset> ActionCatalog;

#if WITH_EDITOR
	UFUNCTION(CallInEditor, Category = "DIVE|Actions", meta = (
		DisplayName = "Add Admin Defaults",
		ToolTip = "Puts Simulate Physics and Delete Mesh into Bindings. Creates the Admin binding if missing, or fills it if you deleted those actions. Safe to click again if they are already there."))
	void AddAdminDefaultBindings();
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Actions", meta = (
		DisplayName = "Bindings",
		ToolTip = "Filled when you add the component (Focus, Isolate, Simulate Physics, Delete Mesh). Delete any binding or action to drop it."))
	TArray<FDIVEActionBinding> Bindings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Actions", meta = (
		DisplayName = "Sections",
		ToolTip = "Menu sections for local Bindings (Standard, Admin)."))
	TArray<FDIVEMenuSection> Sections;

	UPROPERTY(BlueprintAssignable, Category = "DIVE")
	FOnDIVESessionLifecycle OnSessionLifecycle;

	/**
	 * After a successful instant Execute or continuous Begin (session or headless).
	 * NotifyActionExecuted also fans out to Action->OnExecuted (K2 listens here).
	 */
	UPROPERTY(BlueprintAssignable, Category = "DIVE|Actions")
	FOnDIVEActionExecuted OnActionExecuted;

	/**
	 * Live values for this Inspectable's continuous slot (session or headless).
	 * Bind here or via DIVE Action Value Event — not Action->OnValueChanged on the catalog asset.
	 */
	UPROPERTY(BlueprintAssignable, Category = "DIVE|Actions")
	FOnDIVEActionValueChanged OnActionValueChanged;

	void NotifyActionExecuted(UDIVEDeviceAction* Action, const FDIVEActionContext& Context);

	void NotifyActionValueChanged(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		const FDIVEInteractionValue& Value);

	void NotifySessionLifecycle(bool bActive);

	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (DisplayName = "Request Session"))
	bool RequestSession();

	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (DisplayName = "Request Session (With Params)"))
	bool RequestSessionWithParams(const FDIVESessionParams& Params);

	/**
	 * Host ACTS glue: if ActionId is OpenDIVE (`DIVE::kActionOpenDIVE`), begins a session.
	 * Any other id is ignored. Bind from UACTSInteractableComponent::OnActionExecuted.
	 */
	UFUNCTION(BlueprintCallable, Category = "DIVE", meta = (
		DisplayName = "Try Request Session From Action Id"))
	bool TryRequestSessionFromActionId(FName ActionId);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void BuildSemanticRegistry() const;

	/**
	 * Returns available SectionId values for the GetOptions dropdown on Binding.SectionId.
	 * Includes built-in IDs (Standard, Admin) and all authored section IDs from this component and
	 * the linked Action Catalog.
	 */
	UFUNCTION()
	TArray<FName> GetAvailableSectionIds() const;

	/** Non-empty BindingId values from local Bindings and the linked Action Catalog. */
	UFUNCTION()
	TArray<FName> GetAvailableBindingIds() const;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	const FDIVEPartTree& GetSemanticRegistry() const { return SemanticRegistry; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsSessionActive() const;

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

	/** True when a default or per-component hover overlay soft path is authored. */
	UFUNCTION(BlueprintPure, Category = "DIVE|Pick|Hover")
	bool HasPickHoverOverlay() const;

	void PreloadPickHoverOverlays();

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
	FDIVECameraSettings GetEffectiveCameraSettings() const;

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

	/** Interactive device primitives that match Query (same rules as session pick / Scan). */
	void CollectPrimitivesMatchingQuery(
		const FDIVETargetQuery& Query,
		TArray<UPrimitiveComponent*>& OutPrimitives) const;

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
		FName& OutTargetKey,
		FName& OutBindingId) const;

	void AppendConfiguredContextMenuEntries(
		const FDIVEFocusTarget& PickTarget,
		TArray<FDIVEContextMenuEntry>& InOutEntries,
		EDIVEActionPresentation PresentationFilter = EDIVEActionPresentation::Session) const;

	UMaterialInterface* ResolvePickHoverOverlayMaterial(const FDIVEFocusTarget& PickTarget) const;

	/**
	 * Build an action context for the pick. Optional ViewLocation/ViewRotation override the
	 * session camera (pass non-zero / valid rotation from a host ray for headless execute).
	 * When bOverrideView is false, an active session camera is used if present.
	 */
	FDIVEActionContext MakeActionContext(
		const FDIVEFocusTarget& PickTarget,
		FName TargetKey,
		const FVector2D& ScreenPosition = FVector2D::ZeroVector,
		const FHitResult& PickHit = FHitResult(),
		FName BindingId = NAME_None,
		bool bOverrideView = false,
		FVector ViewLocation = FVector::ZeroVector,
		FRotator ViewRotation = FRotator::ZeroRotator) const;

	/**
	 * Headless / host execute: runs Catalog / Bindings actions without an active camera session.
	 * Continuous gestures use this component's slot (separate from the session monitor slot).
	 */
	UFUNCTION(BlueprintCallable, Category = "DIVE|Actions")
	bool ExecuteAction(UDIVEDeviceAction* Action, const FDIVEActionContext& Context);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Actions")
	void UpdateActiveInteraction(const FDIVEInteractionUpdate& Update);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Actions")
	void EndActiveInteraction(bool bCommit = true);

	UFUNCTION(BlueprintPure, Category = "DIVE|Actions")
	bool HasActiveInteraction() const;

	/** Bound by DIVEActionExecution while a continuous gesture is active on this Inspectable. */
	UFUNCTION()
	void HandleContinuousActionValueChanged(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		const FDIVEInteractionValue& Value);

#if WITH_EDITOR
	/**
	 * Device-only authoring checks (warnings): equal-specificity primary overlap,
	 * ComponentTag MatchValues, PartId→anchor coverage, shape pick-channel Block,
	 * catalog-template OnExecuted / OnValueChanged subscribers.
	 * Shared by IsDataValid and DIVE Scan.
	 */
	void AppendDeviceAuthoringValidation(FDataValidationContext& Context) const;

	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	virtual void OnRegister() override;
	virtual void OnComponentCreated() override;
	virtual void PostInitProperties() override;
	virtual void PreSave(FObjectPreSaveContext SaveContext) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UPROPERTY(Transient)
	mutable FDIVEPartTree SemanticRegistry;

	/**
	 * One headless/host continuous gesture at a time on this device (not two-hand).
	 * Separate from the session monitor slot — EndSession must not clear this.
	 */
	FDIVEContinuousActionSlot ContinuousSlot;

	/** Runtime copies of ActionCatalog bindings. Not serialized; rebuilt on register / catalog change. */
	UPROPERTY(Transient)
	TArray<FDIVEActionBinding> InstancedCatalogBindings;

	bool ShouldSkipDefaultBindingSeed() const;
	void SeedDefaultBindingsIfNeeded();
	void EnsureSeededAdminDefaults();
	void InstanceForeignPrivateActions();
	void RebuildInstancedCatalogBindings();
	void DestroyInstancedCatalogActions();
	bool IsInstancedCatalogAction(const UDIVEDeviceAction* Action) const;
	void GatherBindingsForAuthoringValidation(TArray<const FDIVEActionBinding*>& OutBindings) const;

	FName ResolveTargetKeyForQuery(const FDIVETargetQuery& Query, const FDIVEFocusTarget& PickTarget) const;
	bool MatchesComponentNameValue(const UPrimitiveComponent* Primitive, FName MatchValue) const;
	bool IsPrimitiveExcludedFromPickInteraction(const UPrimitiveComponent* Primitive) const;
	bool FindPickHoverOverlaySoftMaterial(const FDIVEFocusTarget& PickTarget, TSoftObjectPtr<UMaterialInterface>& OutSoftMaterial) const;
};
