// Copyright (c) 2026. All Rights Reserved.

#include "DIVEInspectableComponent.h"

#include "DIVEAnchorComponent.h"
#include "DIVEDeviceDefinitionAsset.h"
#include "DIVEHierarchy.h"
#include "DIVESessionSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"

UDIVEInspectableComponent::UDIVEInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UDIVEInspectableComponent::RequestSession()
{
	FDIVESessionParams Params;
	Params.InitialAnchorId = DefaultViewAnchorId;
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
	return OutResult.bSuccess;
}

void UDIVEInspectableComponent::NotifySessionLifecycle(bool bActive)
{
	bSessionActive = bActive;

	if (!bActive)
	{
		UpdateAnchorSessionPresentation(FDIVEFocusTarget::MakeDeviceRoot());
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
