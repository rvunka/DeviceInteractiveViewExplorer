// Copyright (c) 2026. All Rights Reserved.

#include "DIVEInspectableComponent.h"

#include "DIVEAnchorComponent.h"
#include "DIVEDeviceDefinitionAsset.h"
#include "DIVEHierarchy.h"
#include "DIVESessionSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "Utils/DIVEOperations.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

UDIVEInspectableComponent::UDIVEInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UDIVEInspectableComponent::RequestSession()
{
	FDIVESessionParams Params;
	Params.InitialFocusId = DefaultStartFocusId;
	return RequestSessionWithParams(Params);
}

bool UDIVEInspectableComponent::RequestSessionWithParams(const FDIVESessionParams& Params)
{
	if (!GetOwner() || !GetWorld())
	{
		return false;
	}

	UGameInstance* GameInstance = GetWorld()->GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	UDIVESessionSubsystem* Subsystem = GameInstance->GetSubsystem<UDIVESessionSubsystem>();
	return Subsystem ? Subsystem->TryBeginSession(GetOwner(), this, Params) : false;
}

void UDIVEInspectableComponent::BuildSemanticRegistry()
{
	SemanticRegistry.Nodes.Reset();

	TArray<UDIVEAnchorComponent*> Anchors;
	if (GetOwner())
	{
		GetOwner()->GetComponents<UDIVEAnchorComponent>(Anchors);
	}

	for (UDIVEAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor)
		{
			continue;
		}

		const FName ResolvedPartId = Anchor->GetResolvedPartId();
		FDIVEPartNode Node;
		Node.PartId = ResolvedPartId;
		Node.DisplayName = Anchor->DisplayName.IsEmpty()
			? FText::FromName(ResolvedPartId)
			: Anchor->DisplayName;
		Node.OperationIds = Anchor->OperationIds;
		Node.SceneComponent = Anchor;
		SemanticRegistry.Nodes.Add(Node);
	}
}

FName UDIVEInspectableComponent::ResolveSemanticPartId(const UPrimitiveComponent* Primitive) const
{
	if (!Primitive)
	{
		return NAME_None;
	}

	FName BestPartId = NAME_None;
	int32 BestDepth = INDEX_NONE;
	for (const FDIVEPartNode& Node : SemanticRegistry.Nodes)
	{
		if (USceneComponent* AnchorComponent = Node.SceneComponent.Get())
		{
			const int32 Depth = DIVE::GetAttachDepthToAncestor(Primitive, AnchorComponent);
			if (Depth != INDEX_NONE && (BestDepth == INDEX_NONE || Depth < BestDepth))
			{
				BestDepth = Depth;
				BestPartId = Node.PartId;
			}
		}
	}

	return BestPartId;
}

bool UDIVEInspectableComponent::IsPrimitivePickable(const UPrimitiveComponent* Primitive) const
{
	if (!Primitive || !Primitive->IsVisible())
	{
		return false;
	}

	if (!SkipComponentTag.IsNone() && Primitive->ComponentHasTag(SkipComponentTag))
	{
		return false;
	}

	if (MinPickBoundsRadius > 0.f)
	{
		const float MinExtent = Primitive->Bounds.BoxExtent.GetMin();
		if (MinExtent < MinPickBoundsRadius)
		{
			return false;
		}
	}

	return true;
}

float UDIVEInspectableComponent::GetEffectiveOrbitSensitivity() const
{
	if (bUseDeviceDefinitionSettings && DeviceDefinition)
	{
		return DeviceDefinition->OrbitSensitivity;
	}

	return OrbitSensitivity;
}

float UDIVEInspectableComponent::GetEffectiveZoomSensitivity() const
{
	if (bUseDeviceDefinitionSettings && DeviceDefinition)
	{
		return DeviceDefinition->ZoomSensitivity;
	}

	return ZoomSensitivity;
}

float UDIVEInspectableComponent::GetEffectiveDefaultOrbitDistance() const
{
	if (bUseDeviceDefinitionSettings && DeviceDefinition)
	{
		return DeviceDefinition->DefaultOrbitDistance;
	}

	return DefaultOrbitDistance;
}

bool UDIVEInspectableComponent::TryResolveStartFocusTarget(FName FocusObjectId, FDIVEFocusTarget& OutTarget) const
{
	OutTarget = FDIVEFocusTarget::MakeDeviceRoot();

	if (FocusObjectId.IsNone() || !GetOwner())
	{
		return false;
	}

	FDIVEPartNode AnchorNode;
	if (FindAnchorNode(FocusObjectId, AnchorNode))
	{
		if (USceneComponent* AnchorComponent = AnchorNode.SceneComponent.Get())
		{
			OutTarget = FDIVEFocusTarget::FromAnchor(AnchorComponent, AnchorNode.PartId);
			return true;
		}
	}

	AActor* DeviceHost = GetOwner();
	bool bResolved = false;
	DIVE::ForEachDeviceActor(DeviceHost, [this, FocusObjectId, &OutTarget, &bResolved](AActor* Actor)
	{
		if (bResolved)
		{
			return;
		}

		TArray<UPrimitiveComponent*> Primitives;
		Actor->GetComponents<UPrimitiveComponent>(Primitives);

		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (!Primitive || Primitive->GetFName() != FocusObjectId)
			{
				continue;
			}

			if (!IsPrimitivePickable(Primitive))
			{
				return;
			}

			OutTarget = FDIVEFocusTarget::FromPrimitive(Primitive, ResolveSemanticPartId(Primitive));
			bResolved = true;
			return;
		}
	});

	return bResolved;
}

TArray<FName> UDIVEInspectableComponent::ResolveOperationIdsForFocus(const FDIVEFocusTarget& FocusTarget) const
{
	TArray<FName> OperationIds;

	if (FocusTarget.Kind == EDIVEFocusKind::Anchor)
	{
		if (const UDIVEAnchorComponent* Anchor = Cast<UDIVEAnchorComponent>(FocusTarget.Anchor.Get()))
		{
			OperationIds = Anchor->OperationIds;
		}
	}
	else if (!FocusTarget.SemanticPartId.IsNone())
	{
		FDIVEPartNode Node;
		if (FindAnchorNode(FocusTarget.SemanticPartId, Node))
		{
			OperationIds = Node.OperationIds;
		}
	}

	return OperationIds;
}

FDIVEOperationDescriptor UDIVEInspectableComponent::ResolveOperationDescriptor(FName OperationId) const
{
	FDIVEOperationDescriptor Descriptor;
	if (DIVEOperations::FindCatalogDescriptor(DeviceDefinition, OperationId, Descriptor))
	{
		return Descriptor;
	}

	return DIVEOperations::MakeFallbackDescriptor(OperationId);
}

void UDIVEInspectableComponent::GetAvailableOperationsForFocus(
	const FDIVEFocusTarget& FocusTarget,
	TArray<FDIVEOperationDescriptor>& OutOperations) const
{
	OutOperations.Reset();

	const TArray<FName> OperationIds = ResolveOperationIdsForFocus(FocusTarget);
	for (const FName OperationId : OperationIds)
	{
		if (OperationId.IsNone())
		{
			continue;
		}

		OutOperations.Add(ResolveOperationDescriptor(OperationId));
	}
}

bool UDIVEInspectableComponent::ValidateOperation(FName OperationId, FText& OutFailureMessage) const
{
	return DIVEOperations::ValidateOperation(DeviceDefinition, OperationId, CompletedOperationIds, OutFailureMessage);
}

bool UDIVEInspectableComponent::IsOperationCompleted(FName OperationId) const
{
	return !OperationId.IsNone() && CompletedOperationIds.Contains(OperationId);
}

void UDIVEInspectableComponent::ResetSessionOperationState()
{
	CompletedOperationIds.Reset();
}

void UDIVEInspectableComponent::MarkOperationCompleted(FName OperationId)
{
	if (!OperationId.IsNone())
	{
		CompletedOperationIds.Add(OperationId);
	}
}

bool UDIVEInspectableComponent::FindAnchorNode(FName PartId, FDIVEPartNode& OutNode) const
{
	if (PartId.IsNone())
	{
		return false;
	}

	if (const FDIVEPartNode* Node = SemanticRegistry.FindNode(PartId))
	{
		OutNode = *Node;
		return true;
	}

	return false;
}

bool UDIVEInspectableComponent::RequestOperation(const FDIVEOperationRequest& Request, FDIVEOperationResult& OutResult)
{
	OutResult = FDIVEOperationResult();
	OnOperationRequested.Broadcast(Request, OutResult);

	if (!OutResult.bSuccess && OutResult.Message.IsEmpty())
	{
		OutResult.Message = NSLOCTEXT("DIVE", "OperationUnhandled", "No handler accepted this operation.");
	}

	return OutResult.bSuccess;
}

void UDIVEInspectableComponent::NotifySessionLifecycle(bool bActive)
{
	bSessionActive = bActive;

	if (bActive)
	{
		ResetSessionOperationState();

		if (GetOwner())
		{
			TArray<UDIVEAnchorComponent*> Anchors;
			GetOwner()->GetComponents<UDIVEAnchorComponent>(Anchors);
			for (UDIVEAnchorComponent* Anchor : Anchors)
			{
				if (Anchor && Anchor->SupportsManipulation())
				{
					Anchor->CaptureManipulationBase();
				}
			}
		}
	}
	else
	{
		UpdateAnchorSessionPresentation(FDIVEFocusTarget::MakeDeviceRoot());
		ResetSessionOperationState();
	}

	OnSessionLifecycle.Broadcast(bActive);
}

void UDIVEInspectableComponent::UpdateAnchorSessionPresentation(const FDIVEFocusTarget& FocusedTarget)
{
	if (!GetOwner())
	{
		return;
	}

	TArray<UDIVEAnchorComponent*> Anchors;
	GetOwner()->GetComponents<UDIVEAnchorComponent>(Anchors);

	for (UDIVEAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor)
		{
			continue;
		}

		const bool bHidePickMarker = bSessionActive
			&& FocusedTarget.Kind == EDIVEFocusKind::Anchor
			&& FocusedTarget.Anchor.Get() == Anchor;

		Anchor->SetSessionPresentation(bSessionActive, bHidePickMarker);
	}
}

#if WITH_EDITOR

EDataValidationResult UDIVEInspectableComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return Result;
	}

	if (!bUseDeviceDefinitionSettings)
	{
		if (OrbitSensitivity <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("OrbitSensitivity must be greater than zero.")));
			Result = EDataValidationResult::Invalid;
		}

		if (ZoomSensitivity <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("ZoomSensitivity must be greater than zero.")));
			Result = EDataValidationResult::Invalid;
		}

		if (DefaultOrbitDistance <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("DefaultOrbitDistance must be greater than zero.")));
			Result = EDataValidationResult::Invalid;
		}
	}
	else if (!DeviceDefinition)
	{
		Context.AddWarning(FText::FromString(
			TEXT("bUseDeviceDefinitionSettings is enabled but DeviceDefinition is not assigned.")));
	}

	TArray<UDIVEAnchorComponent*> Anchors;
	Owner->GetComponents<UDIVEAnchorComponent>(Anchors);

	TMap<FName, UDIVEAnchorComponent*> PartIdOwners;
	for (UDIVEAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor)
		{
			continue;
		}

		const FName ResolvedPartId = Anchor->GetResolvedPartId();
		if (ResolvedPartId.IsNone())
		{
			Context.AddError(FText::FromString(
				FString::Printf(TEXT("Anchor '%s' resolves to an empty PartId."), *GetNameSafe(Anchor))));
			Result = EDataValidationResult::Invalid;
			continue;
		}

		if (UDIVEAnchorComponent** ExistingOwner = PartIdOwners.Find(ResolvedPartId))
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("Duplicate PartId '%s' on anchors '%s' and '%s'."),
				*ResolvedPartId.ToString(),
				*GetNameSafe(*ExistingOwner),
				*GetNameSafe(Anchor))));
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			PartIdOwners.Add(ResolvedPartId, Anchor);
		}
	}

	if (!DefaultStartFocusId.IsNone())
	{
		FDIVEFocusTarget ResolvedTarget;
		if (!TryResolveStartFocusTarget(DefaultStartFocusId, ResolvedTarget))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("DefaultStartFocusId '%s' does not match any anchor PartId or pickable mesh component on this actor."),
				*DefaultStartFocusId.ToString())));
		}
	}

	return Result;
}

#endif // WITH_EDITOR
