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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|View", meta = (ClampMin = "50.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.0", EditCondition = "!bUseDeviceDefinitionSettings"))
	float FocusBlendDuration = 0.35f;

	/** Default session marker mesh for all anchors (engine sphere unless overridden per anchor). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Anchor", meta = (
		DisplayName = "Default Anchor Marker Mesh",
		ToolTip = "Device-wide marker mesh. Shown in DIVE session on each DIVE Anchor. Per-anchor: Marker Mesh Override."))
	TSoftObjectPtr<UStaticMesh> DefaultAnchorMarkerMesh;

	/** Material for anchor session markers. Color and opacity are authored in this asset (or per-anchor override). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Anchor", meta = (
		DisplayName = "Default Anchor Marker Material",
		ToolTip = "Device-wide marker material. Per-anchor: Marker Material Override."))
	TSoftObjectPtr<UMaterialInterface> DefaultAnchorMarkerMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|Anchor", meta = (
		DisplayName = "Default Anchor Marker Scale",
		ClampMin = "0.01",
		ToolTip = "Device-wide marker size. Per-anchor Marker Scale overrides when not left at plugin default."))
	float DefaultAnchorMarkerScale = 0.12f;

	/** Component name (Components tab) or anchor PartId → custom menu rows. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DIVE|ContextMenu")
	TMap<FName, FDIVEPickContextMenuActionList> PickContextMenuByComponent;

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

	/** Pickable and not listed in Pick Interaction Exclusions. */
	UFUNCTION(BlueprintPure, Category = "DIVE|Pick")
	bool IsPrimitiveInteractive(const UPrimitiveComponent* Primitive) const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveOrbitSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Camera")
	float GetEffectiveZoomSensitivity() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|View")
	float GetEffectiveDefaultOrbitDistance() const;

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
