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

/** Session policy for routing semantic input (Interact / Physical). */
UENUM(BlueprintType)
enum class EDIVESessionInteractionMode : uint8
{
	Interact UMETA(DisplayName = "Interact"),
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

/** Authored and resolved orbit/zoom/focus camera parameters (Inspectable or Device Definition). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVECameraSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.01"))
	float OrbitSensitivity = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.01"))
	float ZoomSensitivity = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ToolTip = "Scale zoom step with orbit distance (finer near, coarser far)."))
	bool bScaleZoomWithOrbitDistance = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ClampMin = "1.0",
		EditCondition = "bScaleZoomWithOrbitDistance",
		ToolTip = "Orbit distance (cm) where Zoom Sensitivity maps 1:1."))
	float ZoomDistanceReferenceCm = DIVE::kDefaultZoomDistanceReference;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0"))
	float MinOrbitDistanceCm = DIVE::kDefaultMinOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0"))
	float MaxOrbitDistanceCm = DIVE::kDefaultMaxOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ClampMin = "1.0",
		ToolTip = "Focus orbit distance ≈ SphereRadius × this."))
	float FocusOrbitFitMultiplier = DIVE::kDefaultFocusOrbitFitMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (
		ClampMin = "0.0",
		ToolTip = "Near zoom floor ≈ SphereRadius × this while focused."))
	float FocusNearPaddingFactor = DIVE::kDefaultFocusNearPaddingFactor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "1.0"))
	float DefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Camera", meta = (ClampMin = "0.0"))
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
