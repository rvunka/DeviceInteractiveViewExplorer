// Copyright (c) 2026. All Rights Reserved.

#include "DIVETypes.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Actor.h"

bool FDIVEFocusTarget::IsValidFocus() const
{
	switch (Kind)
	{
	case EDIVEFocusKind::DeviceRoot:
		return true;
	case EDIVEFocusKind::Primitive:
		return Primitive != nullptr;
	case EDIVEFocusKind::Anchor:
		return Anchor != nullptr;
	default:
		return false;
	}
}

bool FDIVEFocusTarget::Equals(const FDIVEFocusTarget& Other) const
{
	if (Kind != Other.Kind)
	{
		return false;
	}

	switch (Kind)
	{
	case EDIVEFocusKind::DeviceRoot:
		return true;
	case EDIVEFocusKind::Primitive:
		return Primitive == Other.Primitive && SemanticPartId == Other.SemanticPartId;
	case EDIVEFocusKind::Anchor:
		return Anchor == Other.Anchor && SemanticPartId == Other.SemanticPartId;
	default:
		return false;
	}
}

FVector FDIVEFocusTarget::GetPivotLocation(const AActor* DeviceHost) const
{
	switch (Kind)
	{
	case EDIVEFocusKind::Primitive:
		if (Primitive)
		{
			return Primitive->Bounds.Origin;
		}
		break;
	case EDIVEFocusKind::Anchor:
		if (Anchor)
		{
			return Anchor->GetComponentLocation();
		}
		break;
	case EDIVEFocusKind::DeviceRoot:
	default:
		break;
	}

	if (DeviceHost)
	{
		FVector Origin;
		FVector Extent;
		DeviceHost->GetActorBounds(true, Origin, Extent);
		return Origin;
	}

	return FVector::ZeroVector;
}

FDIVEFocusTarget FDIVEFocusTarget::MakeDeviceRoot()
{
	return FDIVEFocusTarget();
}

FDIVEFocusTarget FDIVEFocusTarget::FromPrimitive(UPrimitiveComponent* InPrimitive, FName InSemanticPartId)
{
	FDIVEFocusTarget Target;
	Target.Kind = EDIVEFocusKind::Primitive;
	Target.Primitive = InPrimitive;
	Target.SemanticPartId = InSemanticPartId;
	return Target;
}

FDIVEFocusTarget FDIVEFocusTarget::FromAnchor(USceneComponent* InAnchor, FName InSemanticPartId)
{
	FDIVEFocusTarget Target;
	Target.Kind = EDIVEFocusKind::Anchor;
	Target.Anchor = InAnchor;
	Target.SemanticPartId = InSemanticPartId;
	return Target;
}
