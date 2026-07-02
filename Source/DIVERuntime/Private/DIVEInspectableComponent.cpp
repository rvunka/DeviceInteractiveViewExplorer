// Copyright (c) 2026. All Rights Reserved.

#include "DIVEInspectableComponent.h"

#include "DIVEAnchorComponent.h"
#include "DIVEDeviceDefinitionAsset.h"
#include "DIVEConvention.h"
#include "DIVEHierarchy.h"
#include "DIVESessionSubsystem.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "Utils/DIVEContextMenu.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
bool InvokeActorFunctionWithOptionalTarget(
	AActor* Owner,
	const FName FunctionName,
	UPrimitiveComponent* TargetComponent)
{
	if (!Owner || FunctionName.IsNone())
	{
		return false;
	}

	UFunction* Function = Owner->FindFunction(FunctionName);
	if (!Function)
	{
		return false;
	}

	if (Function->NumParms == 0)
	{
		Owner->ProcessEvent(Function, nullptr);
		return true;
	}

	TArray<uint8> Params;
	Params.SetNumZeroed(FMath::Max(static_cast<int32>(Function->ParmsSize), 1));

	bool bInvoked = false;
	for (TFieldIterator<FProperty> ParamIt(Function); ParamIt; ++ParamIt)
	{
		FProperty* Param = *ParamIt;
		if (!Param || (Param->PropertyFlags & CPF_Parm) == 0 || (Param->PropertyFlags & CPF_ReturnParm) != 0)
		{
			continue;
		}

		if (FObjectProperty* ObjectParam = CastField<FObjectProperty>(Param))
		{
			if (!ObjectParam->PropertyClass->IsChildOf(UPrimitiveComponent::StaticClass()))
			{
				return false;
			}

			ObjectParam->SetObjectPropertyValue(Params.GetData() + ObjectParam->GetOffset_ForUFunction(), TargetComponent);
			bInvoked = true;
			break;
		}

		return false;
	}

	if (!bInvoked)
	{
		return false;
	}

	Owner->ProcessEvent(Function, Params.GetData());
	return true;
}

bool QueryActorBoolFunction(const AActor* Owner, const FName FunctionName, bool& OutValue)
{
	if (!Owner || FunctionName.IsNone())
	{
		return false;
	}

	UFunction* Function = Owner->FindFunction(FunctionName);
	if (!Function)
	{
		return false;
	}

	const FBoolProperty* ReturnProperty = CastField<FBoolProperty>(Function->GetReturnProperty());
	if (!ReturnProperty)
	{
		return false;
	}

	TArray<uint8> Params;
	Params.SetNumZeroed(FMath::Max(static_cast<int32>(Function->ParmsSize), 1));
	const_cast<AActor*>(Owner)->ProcessEvent(Function, Params.GetData());
	OutValue = ReturnProperty->GetPropertyValue_InContainer(Params.GetData());
	return true;
}

bool QueryActorBoolState(const AActor* Owner, const FName StateName, bool& OutValue)
{
	if (QueryActorBoolFunction(Owner, StateName, OutValue))
	{
		return true;
	}

	if (!Owner || StateName.IsNone())
	{
		return false;
	}

	if (const FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Owner->GetClass(), StateName))
	{
		OutValue = BoolProperty->GetPropertyValue_InContainer(Owner);
		return true;
	}

	return false;
}

bool TryInvokePickContextMenuHandler(
	AActor* Owner,
	const FName ComponentName,
	const FName LocalActionId,
	UPrimitiveComponent* TargetComponent)
{
	return InvokeActorFunctionWithOptionalTarget(
		Owner,
		DIVE::MakePickContextMenuHandlerName(ComponentName, LocalActionId),
		TargetComponent);
}

bool TryQueryPickContextMenuActiveState(
	const AActor* Owner,
	const FName ComponentName,
	const FName LocalActionId,
	bool& OutActive)
{
	return QueryActorBoolState(
		Owner,
		DIVE::MakePickContextMenuActiveStateName(ComponentName, LocalActionId),
		OutActive);
}
} // namespace

UDIVEInspectableComponent::UDIVEInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DefaultAnchorMarkerMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DIVE::DefaultAnchorMarkerMeshPath()));
	DefaultAnchorMarkerMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DIVE::DefaultAnchorMarkerMaterialPath()));
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

	for (const TPair<FName, FDIVEPickContextMenuActionList>& ComponentEntry : PickContextMenuByComponent)
	{
		const FName ComponentName = ComponentEntry.Key;
		const TArray<FDIVEPickContextMenuAction>& Actions = ComponentEntry.Value.Actions;

		if (ComponentName.IsNone())
		{
			Context.AddWarning(FText::FromString(
				TEXT("PickContextMenuByComponent has an entry with an empty component name key and will never match a pick.")));
			continue;
		}

		FDIVEFocusTarget ResolvedTarget;
		if (!TryResolveStartFocusTarget(ComponentName, ResolvedTarget))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("PickContextMenuByComponent key '%s' does not match any anchor PartId or pickable mesh component on this actor."),
				*ComponentName.ToString())));
		}

		if (Actions.IsEmpty())
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("PickContextMenuByComponent '%s' has no actions."),
				*ComponentName.ToString())));
		}

		for (const FDIVEPickContextMenuAction& Action : Actions)
		{
			if (Action.ActionId.IsNone())
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent '%s' has an entry with an empty ActionId."),
					*ComponentName.ToString())));
				Result = EDataValidationResult::Invalid;
				continue;
			}

			if (Action.ActionId == DIVE::kContextFocus
				|| Action.ActionId == DIVE::kContextIsolate
				|| Action.ActionId == DIVE::kContextToggleMeshPhysics
				|| Action.ActionId == DIVE::kContextDeleteMesh)
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent ActionId '%s' is reserved by DIVE built-in menu rows."),
					*Action.ActionId.ToString())));
				Result = EDataValidationResult::Invalid;
			}

			if (Action.DisplayName.IsEmpty())
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent '%s' has an entry with an empty DisplayName."),
					*ComponentName.ToString())));
			}
		}
	}

	TSet<FName> QualifiedActionIds;
	for (const TPair<FName, FDIVEPickContextMenuActionList>& ComponentEntry : PickContextMenuByComponent)
	{
		const FName ComponentName = ComponentEntry.Key;
		if (ComponentName.IsNone())
		{
			continue;
		}

		for (const FDIVEPickContextMenuAction& Action : ComponentEntry.Value.Actions)
		{
			if (Action.ActionId.IsNone())
			{
				continue;
			}

			const FName QualifiedActionId = DIVE::MakeQualifiedPickContextMenuActionId(ComponentName, Action.ActionId);
			if (QualifiedActionIds.Contains(QualifiedActionId))
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("Duplicate qualified ActionId '%s' in PickContextMenuByComponent."),
					*QualifiedActionId.ToString())));
				Result = EDataValidationResult::Invalid;
			}
			else
			{
				QualifiedActionIds.Add(QualifiedActionId);
			}
		}
	}

	return Result;
}

#endif // WITH_EDITOR

bool UDIVEInspectableComponent::NotifyPickContextMenuAction(
	const FName QualifiedActionId,
	const FDIVEFocusTarget& PickTarget)
{
	FName ComponentName = NAME_None;
	FName LocalActionId = NAME_None;
	if (!ResolvePickContextMenuAction(QualifiedActionId, PickTarget, ComponentName, LocalActionId))
	{
		return false;
	}

	UPrimitiveComponent* TargetComponent = PickTarget.Primitive.Get();
	if (AActor* Owner = GetOwner())
	{
		if (TryInvokePickContextMenuHandler(Owner, ComponentName, LocalActionId, TargetComponent))
		{
			return true;
		}
	}

	return false;
}

bool UDIVEInspectableComponent::FindPickContextMenuCatalog(
	const FDIVEFocusTarget& PickTarget,
	FName& OutComponentName,
	const TArray<FDIVEPickContextMenuAction>*& OutActions) const
{
	OutComponentName = NAME_None;
	OutActions = nullptr;

	if (PickTarget.Kind != EDIVEFocusKind::Primitive || PickContextMenuByComponent.IsEmpty())
	{
		return false;
	}

	if (const UPrimitiveComponent* Primitive = PickTarget.Primitive.Get())
	{
		if (const FDIVEPickContextMenuActionList* Found = PickContextMenuByComponent.Find(Primitive->GetFName()))
		{
			OutComponentName = Primitive->GetFName();
			OutActions = &Found->Actions;
			return true;
		}
	}

	if (!PickTarget.SemanticPartId.IsNone())
	{
		if (const FDIVEPickContextMenuActionList* Found = PickContextMenuByComponent.Find(PickTarget.SemanticPartId))
		{
			OutComponentName = PickTarget.SemanticPartId;
			OutActions = &Found->Actions;
			return true;
		}
	}

	return false;
}

bool UDIVEInspectableComponent::ResolvePickContextMenuAction(
	const FName QualifiedActionId,
	const FDIVEFocusTarget& PickTarget,
	FName& OutComponentName,
	FName& OutLocalActionId) const
{
	OutComponentName = NAME_None;
	OutLocalActionId = NAME_None;

	if (QualifiedActionId.IsNone())
	{
		return false;
	}

	FName MatchedComponentName = NAME_None;
	const TArray<FDIVEPickContextMenuAction>* Actions = nullptr;
	if (!FindPickContextMenuCatalog(PickTarget, MatchedComponentName, Actions) || !Actions)
	{
		return false;
	}

	for (const FDIVEPickContextMenuAction& Action : *Actions)
	{
		if (Action.ActionId.IsNone())
		{
			continue;
		}

		if (DIVE::MakeQualifiedPickContextMenuActionId(MatchedComponentName, Action.ActionId) == QualifiedActionId)
		{
			OutComponentName = MatchedComponentName;
			OutLocalActionId = Action.ActionId;
			return true;
		}
	}

	return false;
}

void UDIVEInspectableComponent::AppendConfiguredPickContextMenuEntries(
	const FDIVEFocusTarget& PickTarget,
	TArray<FDIVEContextMenuEntry>& InOutEntries) const
{
	FName ComponentName = NAME_None;
	const TArray<FDIVEPickContextMenuAction>* Actions = nullptr;
	if (!FindPickContextMenuCatalog(PickTarget, ComponentName, Actions) || !Actions || Actions->IsEmpty())
	{
		return;
	}

	AActor* Owner = GetOwner();
	InOutEntries.Reserve(InOutEntries.Num() + Actions->Num());

	for (const FDIVEPickContextMenuAction& Action : *Actions)
	{
		if (Action.ActionId.IsNone())
		{
			continue;
		}

		FDIVEContextMenuEntry Entry;
		Entry.ActionId = DIVE::MakeQualifiedPickContextMenuActionId(ComponentName, Action.ActionId);
		Entry.DisplayName = Action.DisplayName;
		if (Action.bToggleActiveSuffix && Owner)
		{
			bool bActive = false;
			if (TryQueryPickContextMenuActiveState(Owner, ComponentName, Action.ActionId, bActive))
			{
				Entry.DisplayName = DIVEContextMenu::FormatActiveLabelSuffix(Action.DisplayName, bActive);
			}
		}
		Entry.bEnabled = Action.bEnabled;
		InOutEntries.Add(Entry);
	}
}
