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
enum class EDIVEFocusKind : uint8
{
	DeviceRoot,
	Primitive,
	Anchor
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	TArray<FName> OperationIds;

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
	FName InitialAnchorId = NAME_None;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEOperationRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FName OperationId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FName SemanticPartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FDIVEFocusTarget FocusTarget;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEOperationResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	bool bSuccess = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FText Message;
};
