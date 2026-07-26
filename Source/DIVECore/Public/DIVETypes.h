// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DIVEConvention.h"
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

/** One custom context-menu row. Single source of truth; dispatched via IDIVEDeviceActionHandler. */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPickContextMenuAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Operation id for HandleDeviceAction (e.g. Toggle, Unscrew, Open)."))
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Menu label. Empty = ActionId."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "When true, DIVE treats the row as a toggle (Is_* suffix / flip after handler)."))
	bool bToggleActiveSuffix = false;

	FText GetResolvedDisplayName() const
	{
		if (!DisplayName.IsEmpty())
		{
			return DisplayName;
		}
		return ActionId.IsNone() ? FText::GetEmpty() : FText::FromName(ActionId);
	}
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
};

/** Action list value for PickContextMenuByComponent (TMap value; UHT does not allow TArray as map value). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPickContextMenuActionList
{
	GENERATED_BODY()

	/** Must match ActionId of one enabled Actions row; primary invokes the same handler. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Must match ActionId of one enabled Actions row."))
	FName PrimaryActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	TArray<FDIVEPickContextMenuAction> Actions;
};
