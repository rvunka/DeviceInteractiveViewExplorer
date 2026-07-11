// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
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

/** One custom context-menu row (dispatched via Handle_{ComponentKey}_{ActionId} on the device actor). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPickContextMenuAction
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "Short id within this component's action list (e.g. Unscrew). Invoked as Handle_{map key}_{ActionId} on the device actor."))
	FName ActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	bool bEnabled = true;

	/** When true, appends "*" to DisplayName if device actor has bool function or variable Is_{Key}_{ActionId} that is true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	bool bToggleActiveSuffix = false;
};

/** Action list value for PickContextMenuByComponent (TMap value; UHT does not allow TArray as map value). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEPickContextMenuActionList
{
	GENERATED_BODY()

	/** ActionId from Actions invoked by primary action in Default mode. Must match exactly one row when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu", meta = (
		ToolTip = "One ActionId from Actions below. Primary action (IA_DIVE_PrimaryAction → HandlePrimaryAction*) in Default mode invokes the same Handle_{key}_{ActionId} as the context menu row."))
	FName PrimaryActionId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|ContextMenu")
	TArray<FDIVEPickContextMenuAction> Actions;
};
