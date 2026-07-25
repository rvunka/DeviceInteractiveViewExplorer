// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DIVEConvention.h"
#include "DIVEDeviceActionDefinition.h"
#include "DIVETypes.generated.h"

UENUM(BlueprintType)
enum class EDIVESessionState : uint8
{
	Inactive,
	Active
};

UENUM(BlueprintType)
enum class EDIVESessionEndReason : uint8
{
	UserExit,
	SessionRestart,
	Forced
};

UENUM(BlueprintType)
enum class EDIVEFocusKind : uint8
{
	DeviceRoot,
	Primitive,
	Anchor
};

/** Session policy for routing semantic input (Default / Physical). */
UENUM(BlueprintType)
enum class EDIVESessionInteractionMode : uint8
{
	Default UMETA(DisplayName = "Default"),
	Physical UMETA(DisplayName = "Physical")
};

class UPrimitiveComponent;
class USceneComponent;
class AActor;

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEFocusTarget
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	EDIVEFocusKind Kind = EDIVEFocusKind::DeviceRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	TObjectPtr<UPrimitiveComponent> Primitive = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	TObjectPtr<USceneComponent> Anchor = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FName SemanticPartId = NAME_None;

	bool IsValidFocus() const;

	bool Equals(const FDIVEFocusTarget& Other) const;

	FVector GetPivotLocation(const AActor* DeviceHost) const;

	static FDIVEFocusTarget MakeDeviceRoot();

	static FDIVEFocusTarget FromPrimitive(UPrimitiveComponent* InPrimitive, FName InSemanticPartId = NAME_None);

	static FDIVEFocusTarget FromAnchor(USceneComponent* InAnchor, FName InSemanticPartId);
};

/** Resolved orbit/zoom/focus camera parameters (Inspectable or DeviceDefinition). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVECameraEffectiveSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float OrbitSensitivity = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float ZoomSensitivity = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	bool bScaleZoomWithOrbitDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float ZoomDistanceReferenceCm = DIVE::kDefaultZoomDistanceReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float MinOrbitDistanceCm = DIVE::kDefaultMinOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float MaxOrbitDistanceCm = DIVE::kDefaultMaxOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float FocusOrbitFitMultiplier = DIVE::kDefaultFocusOrbitFitMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float FocusNearPaddingFactor = DIVE::kDefaultFocusNearPaddingFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera")
	float FocusBlendDuration = 0.35f;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPartNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FText DisplayName;

	TWeakObjectPtr<USceneComponent> SceneComponent;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPartTree
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	TArray<FDIVEPartNode> Nodes;

	const FDIVEPartNode* FindNode(FName PartId) const
	{
		return Nodes.FindByPredicate([PartId](const FDIVEPartNode& Node)
		{
			return Node.PartId == PartId;
		});
	}
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVESessionParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FName InitialFocusId = NAME_None;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEContextMenuEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	bool bEnabled = true;

	/** When true, renders a section divider instead of a clickable row. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	bool bIsSeparator = false;
};

/** Per-row overrides when a Definition is assigned (INDEX_NONE = use definition default). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEDeviceActionInstanceOverrides
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Override Unscrew turn count. INDEX_NONE (-1) = use UDIVEUnscrewActionDefinition::DefaultTurnCount."))
	int32 UnscrewTurnCount = INDEX_NONE;
};

/** One custom context-menu row (IDIVEDeviceActionHandler preferred; legacy Handle_{Key}_{ActionId} fallback). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPickContextMenuAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Preferred: shared action DataAsset (ActionId + defaults). When set, row ActionId is ignored for dispatch."))
	TObjectPtr<UDIVEDeviceActionDefinition> Definition = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Legacy: short id when Definition is null (e.g. Unscrew). Prefer Definition for new devices."))
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Menu label. Empty = Definition DefaultDisplayName, else ActionId."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	bool bEnabled = true;

	/** Legacy toggle when Definition is null. With Definition, toggle comes from Definition->bToggleActiveSuffix. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Legacy only (no Definition): handler runs on current Is_*, then DIVE flips Is_*. With Definition, use Definition.bToggleActiveSuffix."))
	bool bToggleActiveSuffix = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	FDIVEDeviceActionInstanceOverrides InstanceOverrides;
};

/** Resolved catalog row for Blueprint helpers / diagnostics. */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEResolvedPickAction
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	FName CatalogKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	FName ActionId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	bool bToggleActiveSuffix = false;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	int32 UnscrewTurnCount = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	TObjectPtr<UDIVEDeviceActionDefinition> Definition = nullptr;
};

/** Action list value for PickContextMenuByComponent (TMap value; UHT does not allow TArray as map value). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPickContextMenuActionList
{
	GENERATED_BODY()

	/** Resolved ActionId from Actions invoked by primary action in Default mode. Must match exactly one row when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "One resolved ActionId from Actions below (from Definition or legacy ActionId). Primary action invokes the same handler as the menu row."))
	FName PrimaryActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	TArray<FDIVEPickContextMenuAction> Actions;
};
