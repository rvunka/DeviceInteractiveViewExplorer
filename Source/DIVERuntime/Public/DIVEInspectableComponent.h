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
struct FDIVESessionPickOps;

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

	/**
	 * Hidden mesh / custom primitives with this tag remain pickable (Box/Sphere/Capsule
	 * shapes are pickable without the tag when they have query collision).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (
		DisplayName = "Pick Proxy Component Tag",
		ToolTip = "Tag for Hidden-in-Game meshes used as DIVE hit volumes. Shape components need no tag."))
	FName PickProxyComponentTag = DIVE::kPickProxyTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (ClampMin = "0.0"))
	float MinPickBoundsRadius = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (
		DisplayName = "Pick Trace Channel",
		ToolTip = "Channel used by DIVE screen picks. Default Visibility — set that response to Block on Box/Sphere volumes. Not a separate project channel named Pick."))
	TEnumAsByte<ECollisionChannel> PickTraceChannel = ECC_Visibility;

	/** Component names (Components tab) or anchor PartIds with no pick interaction: no menu, handlers, primary action, or hover. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick", meta = (
		DisplayName = "Pick Interaction Exclusions",
		ToolTip = "Meshes matching these keys are ignored by DIVE pick (same key rules as context menu catalog: component name, then semantic PartId)."))
	TArray<FName> PickInteractionExclusions;

	/** Device-wide hover overlay in Default mode. Per-component overrides: Pick Hover Overlay By Component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick|Hover", meta = (
		DisplayName = "Hover Overlay Material",
		ToolTip = "Default overlay material for interactive pickable meshes under the cursor. Overridden per mesh in Pick Hover Overlay By Component."))
	TSoftObjectPtr<UMaterialInterface> DefaultPickHoverOverlayMaterial;

	/** Component name or PartId → hover overlay material override. Empty map value uses Hover Overlay Material above. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Pick|Hover", meta = (
		DisplayName = "Pick Hover Overlay By Component"))
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

	/** When true, zoom step scales with current orbit distance (finer near, coarser far). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (EditCondition = "!bUseDeviceDefinitionSettings"))
	bool bScaleZoomWithOrbitDistance = true;

	/** Orbit distance (cm) at which ZoomSensitivity maps 1:1 to one zoom step. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ClampMin = "1.0",
		EditCondition = "!bUseDeviceDefinitionSettings && bScaleZoomWithOrbitDistance"))
	float ZoomDistanceReferenceCm = DIVE::kDefaultZoomDistanceReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float MinOrbitDistanceCm = DIVE::kDefaultMinOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float MaxOrbitDistanceCm = DIVE::kDefaultMaxOrbitDistance;

	/** Focused primitive: OrbitDistance ≈ SphereRadius × this (clamped to min/max). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float FocusOrbitFitMultiplier = DIVE::kDefaultFocusOrbitFitMultiplier;

	/** Focused primitive: near zoom floor ≈ SphereRadius × this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float FocusNearPaddingFactor = DIVE::kDefaultFocusNearPaddingFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|View", meta = (ClampMin = "1.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float FocusBlendDuration = 0.35f;

	/** Map key must equal the component object name from the Components panel (or anchor PartId). Built-in Focus/Isolate ignore this map. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Key = exact component name (not a label). Duplicate often yields Switch1_1 — rename before adding a catalog key. Matching also strips _GEN_VARIABLE and trailing _N. Runtime: DIVE.DumpDevice."))
	TMap<FName, FDIVEPickContextMenuActionList> PickContextMenuByComponent;

	/**
	 * When true (and not a Shipping build), context menu includes Simulate Physics / Delete Mesh.
	 * Off by default — these are administrator/dev operations, not end-user actions.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		DisplayName = "Enable Admin Context Menu Entries",
		ToolTip = "Adds Simulate Physics and Delete Mesh for mesh picks. Ignored in Shipping builds."))
	bool bEnableAdminContextMenuEntries = false;

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

	void AppendConfiguredPickContextMenuEntries(const FDIVEFocusTarget& PickTarget, TArray<FDIVEContextMenuEntry>& InOutEntries) const;

	bool TryResolvePrimaryPickAction(const FDIVEFocusTarget& PickTarget, FName& OutQualifiedActionId) const;
	UMaterialInterface* ResolvePickHoverOverlayMaterial(const FDIVEFocusTarget& PickTarget) const;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool FindAnchorNode(FName PartId, FDIVEPartNode& OutNode) const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

private:
	UPROPERTY(Transient)
	FDIVEPartTree SemanticRegistry;

	UPROPERTY(Transient)
	bool bSessionActive = false;

	friend class UDIVESessionSubsystem;
	friend struct FDIVESessionPickOps;

	void NotifySessionLifecycle(bool bActive);
	bool NotifyPickContextMenuAction(FName QualifiedActionId, const FDIVEFocusTarget& PickTarget);

	bool FindPickContextMenuCatalog(
		const FDIVEFocusTarget& PickTarget,
		FName& OutComponentName,
		const FDIVEPickContextMenuActionList*& OutCatalog) const;

	bool ResolvePickContextMenuAction(
		FName QualifiedActionId,
		const FDIVEFocusTarget& PickTarget,
		FName& OutComponentName,
		FName& OutLocalActionId) const;

	bool IsPrimitiveExcludedFromPickInteraction(const UPrimitiveComponent* Primitive) const;
	bool FindPickHoverOverlaySoftMaterial(const FDIVEFocusTarget& PickTarget, TSoftObjectPtr<UMaterialInterface>& OutSoftMaterial) const;
};
