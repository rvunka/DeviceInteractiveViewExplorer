// Copyright (c) 2026. All Rights Reserved.

#include "DIVEInspectableComponent.h"

#include "DIVEAnchorComponent.h"
#include "DIVEDeviceActionHandler.h"
#include "DIVEDeviceActionResolve.h"
#include "DIVEDeviceDefinitionAsset.h"
#include "DIVEConvention.h"
#include "DIVEHierarchy.h"
#include "DIVELog.h"
#include "DIVESessionSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/GameInstance.h"
#include "Utils/DIVEContextMenu.h"
#include "UObject/ObjectKey.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

namespace
{
const FBoolProperty* FindActorBoolProperty(const AActor* Owner, const FName PropertyName)
{
	if (!Owner || PropertyName.IsNone())
	{
		return nullptr;
	}

	if (const FBoolProperty* Direct = FindFProperty<FBoolProperty>(Owner->GetClass(), PropertyName))
	{
		return Direct;
	}

	// Blueprint variables sometimes only match via authored/display name walk.
	const FString Wanted = PropertyName.ToString();
	for (TFieldIterator<FBoolProperty> It(Owner->GetClass()); It; ++It)
	{
		const FBoolProperty* BoolProperty = *It;
		if (!BoolProperty)
		{
			continue;
		}

		if (BoolProperty->GetFName() == PropertyName
			|| BoolProperty->GetName() == Wanted
			|| BoolProperty->GetAuthoredName() == Wanted)
		{
			return BoolProperty;
		}
	}

	return nullptr;
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
	// Prefer bool property over a same-named pure function (menu "*" must match the variable).
	if (const FBoolProperty* BoolProperty = FindActorBoolProperty(Owner, StateName))
	{
		OutValue = BoolProperty->GetPropertyValue_InContainer(Owner);
		return true;
	}

	return QueryActorBoolFunction(Owner, StateName, OutValue);
}

bool TryToggleActorBoolProperty(AActor* Owner, const FName PropertyName, bool& OutNewValue)
{
	OutNewValue = false;
	const FBoolProperty* BoolProperty = FindActorBoolProperty(Owner, PropertyName);
	if (!BoolProperty || !Owner)
	{
		return false;
	}

	const bool bCurrent = BoolProperty->GetPropertyValue_InContainer(Owner);
	const bool bNew = !bCurrent;
	BoolProperty->SetPropertyValue_InContainer(Owner, bNew);

	// Prefer Blueprint setter if present (keeps BP graphs / accessors in sync).
	const FName SetterName(*FString::Printf(TEXT("set_%s"), *PropertyName.ToString()));
	if (UFunction* Setter = Owner->FindFunction(SetterName))
	{
		TArray<uint8> Params;
		Params.SetNumZeroed(FMath::Max(static_cast<int32>(Setter->ParmsSize), 1));
		for (TFieldIterator<FProperty> ParamIt(Setter); ParamIt; ++ParamIt)
		{
			FProperty* Param = *ParamIt;
			if (!Param || (Param->PropertyFlags & CPF_Parm) == 0 || (Param->PropertyFlags & CPF_ReturnParm) != 0)
			{
				continue;
			}

			if (FBoolProperty* BoolParam = CastField<FBoolProperty>(Param))
			{
				BoolParam->SetPropertyValue(Params.GetData() + BoolParam->GetOffset_ForUFunction(), bNew);
				break;
			}
		}
		Owner->ProcessEvent(Setter, Params.GetData());
	}

	OutNewValue = BoolProperty->GetPropertyValue_InContainer(Owner);
	return true;
}

bool InvokeActorFunctionWithOptionalTarget(
	AActor* Owner,
	const FName FunctionName,
	UPrimitiveComponent* TargetComponent,
	const bool* OptionalActiveState)
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

	TArray<uint8> Params;
	const bool bHasParams = Function->ParmsSize > 0;
	if (bHasParams)
	{
		Params.SetNumZeroed(Function->ParmsSize);
	}

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
			continue;
		}

		if (FBoolProperty* BoolParam = CastField<FBoolProperty>(Param))
		{
			if (OptionalActiveState)
			{
				BoolParam->SetPropertyValue(
					Params.GetData() + BoolParam->GetOffset_ForUFunction(),
					*OptionalActiveState);
			}
			continue;
		}

		return false;
	}

	Owner->ProcessEvent(Function, bHasParams ? Params.GetData() : nullptr);
	return true;
}

bool TryInvokePickContextMenuHandler(
	AActor* Owner,
	const FName ComponentName,
	const FName LocalActionId,
	UPrimitiveComponent* TargetComponent,
	const bool* OptionalActiveState)
{
	if (Owner && Owner->Implements<UDIVEDeviceActionHandler>())
	{
		const bool bActiveBefore = OptionalActiveState ? *OptionalActiveState : false;
		if (IDIVEDeviceActionHandler::Execute_HandleDeviceAction(
				Owner,
				ComponentName,
				LocalActionId,
				TargetComponent,
				bActiveBefore))
		{
			return true;
		}
	}

	const FName LegacyHandlerName = DIVE::MakePickContextMenuHandlerName(ComponentName, LocalActionId);
	if (Owner && Owner->FindFunction(LegacyHandlerName))
	{
		static TSet<FObjectKey> WarnedLegacyHandleOwners;
		const FObjectKey OwnerKey(Owner);
		if (!WarnedLegacyHandleOwners.Contains(OwnerKey))
		{
			WarnedLegacyHandleOwners.Add(OwnerKey);
			UE_LOG(
				LogDIVE,
				Warning,
				TEXT("DIVE: '%s' dispatched via legacy %s — prefer IDIVEDeviceActionHandler (HandleDeviceAction)."),
				*GetNameSafe(Owner),
				*LegacyHandlerName.ToString());
		}
	}

	return InvokeActorFunctionWithOptionalTarget(
		Owner,
		LegacyHandlerName,
		TargetComponent,
		OptionalActiveState);
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

bool TryTogglePickContextMenuActiveState(
	AActor* Owner,
	const FName ComponentName,
	const FName LocalActionId,
	bool& OutNewValue)
{
	return TryToggleActorBoolProperty(
		Owner,
		DIVE::MakePickContextMenuActiveStateName(ComponentName, LocalActionId),
		OutNewValue);
}
} // namespace

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
	if (!Primitive)
	{
		return false;
	}

	if (!SkipComponentTag.IsNone() && Primitive->ComponentHasTag(SkipComponentTag))
	{
		return false;
	}

	const bool bVisible = Primitive->IsVisible();
	const bool bPickProxy = IsPickProxyPrimitive(Primitive);

	if (!bVisible && !bPickProxy)
	{
		return false;
	}

	if (!bVisible && bPickProxy && Primitive->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
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

bool UDIVEInspectableComponent::IsPickProxyPrimitive(const UPrimitiveComponent* Primitive) const
{
	if (!Primitive)
	{
		return false;
	}

	if (Primitive->IsA(UShapeComponent::StaticClass()))
	{
		return true;
	}

	return !PickProxyComponentTag.IsNone() && Primitive->ComponentHasTag(PickProxyComponentTag);
}

bool UDIVEInspectableComponent::IsPrimitiveExcludedFromPickInteraction(const UPrimitiveComponent* Primitive) const
{
	if (!Primitive || PickInteractionExclusions.IsEmpty())
	{
		return false;
	}

	if (PickInteractionExclusions.Contains(Primitive->GetFName()))
	{
		return true;
	}

	const FName SemanticPartId = ResolveSemanticPartId(Primitive);
	return !SemanticPartId.IsNone() && PickInteractionExclusions.Contains(SemanticPartId);
}

bool UDIVEInspectableComponent::IsPrimitiveInteractive(const UPrimitiveComponent* Primitive) const
{
	return IsPrimitivePickable(Primitive) && !IsPrimitiveExcludedFromPickInteraction(Primitive);
}

float UDIVEInspectableComponent::GetEffectiveOrbitSensitivity() const
{
	return GetEffectiveCameraSettings().OrbitSensitivity;
}

float UDIVEInspectableComponent::GetEffectiveZoomSensitivity() const
{
	return GetEffectiveCameraSettings().ZoomSensitivity;
}

bool UDIVEInspectableComponent::GetEffectiveScaleZoomWithOrbitDistance() const
{
	return GetEffectiveCameraSettings().bScaleZoomWithOrbitDistance;
}

float UDIVEInspectableComponent::GetEffectiveZoomDistanceReferenceCm() const
{
	return GetEffectiveCameraSettings().ZoomDistanceReferenceCm;
}

float UDIVEInspectableComponent::GetEffectiveMinOrbitDistanceCm() const
{
	return GetEffectiveCameraSettings().MinOrbitDistanceCm;
}

float UDIVEInspectableComponent::GetEffectiveMaxOrbitDistanceCm() const
{
	return GetEffectiveCameraSettings().MaxOrbitDistanceCm;
}

float UDIVEInspectableComponent::GetEffectiveFocusOrbitFitMultiplier() const
{
	return GetEffectiveCameraSettings().FocusOrbitFitMultiplier;
}

float UDIVEInspectableComponent::GetEffectiveFocusNearPaddingFactor() const
{
	return GetEffectiveCameraSettings().FocusNearPaddingFactor;
}

float UDIVEInspectableComponent::GetEffectiveDefaultOrbitDistance() const
{
	return GetEffectiveCameraSettings().DefaultOrbitDistance;
}

FDIVECameraEffectiveSettings UDIVEInspectableComponent::GetEffectiveCameraSettings() const
{
	FDIVECameraEffectiveSettings Settings;
	Settings.OrbitSensitivity = OrbitSensitivity;
	Settings.ZoomSensitivity = ZoomSensitivity;
	Settings.bScaleZoomWithOrbitDistance = bScaleZoomWithOrbitDistance;
	Settings.ZoomDistanceReferenceCm = ZoomDistanceReferenceCm;
	Settings.MinOrbitDistanceCm = MinOrbitDistanceCm;
	Settings.MaxOrbitDistanceCm = MaxOrbitDistanceCm;
	Settings.FocusOrbitFitMultiplier = FocusOrbitFitMultiplier;
	Settings.FocusNearPaddingFactor = FocusNearPaddingFactor;
	Settings.DefaultOrbitDistance = DefaultOrbitDistance;
	Settings.FocusBlendDuration = FocusBlendDuration;

	if (bUseDeviceDefinitionSettings && DeviceDefinition)
	{
		Settings.OrbitSensitivity = DeviceDefinition->OrbitSensitivity;
		Settings.ZoomSensitivity = DeviceDefinition->ZoomSensitivity;
		Settings.bScaleZoomWithOrbitDistance = DeviceDefinition->bScaleZoomWithOrbitDistance;
		Settings.ZoomDistanceReferenceCm = DeviceDefinition->ZoomDistanceReferenceCm;
		Settings.MinOrbitDistanceCm = DeviceDefinition->MinOrbitDistanceCm;
		Settings.MaxOrbitDistanceCm = DeviceDefinition->MaxOrbitDistanceCm;
		Settings.FocusOrbitFitMultiplier = DeviceDefinition->FocusOrbitFitMultiplier;
		Settings.FocusNearPaddingFactor = DeviceDefinition->FocusNearPaddingFactor;
		Settings.DefaultOrbitDistance = DeviceDefinition->DefaultOrbitDistance;
	}

	return Settings;
}

float UDIVEInspectableComponent::ComputeOrbitDistanceForFocus(const FDIVEFocusTarget& Target) const
{
	const float MinDistance = GetEffectiveMinOrbitDistanceCm();
	const float MaxDistance = FMath::Max(GetEffectiveMaxOrbitDistanceCm(), MinDistance);

	if (Target.Kind == EDIVEFocusKind::Primitive)
	{
		if (const UPrimitiveComponent* Primitive = Target.Primitive.Get())
		{
			const float Radius = FMath::Max(Primitive->Bounds.SphereRadius, 1.f);
			return FMath::Clamp(Radius * GetEffectiveFocusOrbitFitMultiplier(), MinDistance, MaxDistance);
		}
	}

	return FMath::Clamp(GetEffectiveDefaultOrbitDistance(), MinDistance, MaxDistance);
}

float UDIVEInspectableComponent::ComputeFocusClearanceRadius(const FDIVEFocusTarget& Target) const
{
	if (Target.Kind != EDIVEFocusKind::Primitive)
	{
		return 0.f;
	}

	const UPrimitiveComponent* Primitive = Target.Primitive.Get();
	if (!Primitive)
	{
		return 0.f;
	}

	const float Radius = FMath::Max(Primitive->Bounds.SphereRadius, 0.f);
	return Radius * GetEffectiveFocusNearPaddingFactor();
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
				continue;
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

	// Validation / early resolve can run before BuildSemanticRegistry — scan live anchors.
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	TArray<UDIVEAnchorComponent*> Anchors;
	Owner->GetComponents<UDIVEAnchorComponent>(Anchors);
	for (UDIVEAnchorComponent* Anchor : Anchors)
	{
		if (!Anchor)
		{
			continue;
		}

		const FName ResolvedPartId = Anchor->GetResolvedPartId();
		if (ResolvedPartId == PartId || Anchor->GetFName() == PartId)
		{
			OutNode.PartId = ResolvedPartId;
			OutNode.DisplayName = Anchor->DisplayName.IsEmpty()
				? FText::FromName(ResolvedPartId)
				: Anchor->DisplayName;
			OutNode.SceneComponent = Anchor;
			return true;
		}
	}

	return false;
}

void UDIVEInspectableComponent::NotifySessionLifecycle(bool bActive)
{
	bSessionActive = bActive;
	OnSessionLifecycle.Broadcast(bActive);
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

		if (MinOrbitDistanceCm <= 0.f || MaxOrbitDistanceCm < MinOrbitDistanceCm)
		{
			Context.AddError(FText::FromString(TEXT("Orbit distance limits must satisfy 0 < Min <= Max.")));
			Result = EDataValidationResult::Invalid;
		}

		if (bScaleZoomWithOrbitDistance && ZoomDistanceReferenceCm <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("ZoomDistanceReferenceCm must be greater than zero.")));
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

		if (PickInteractionExclusions.Contains(ComponentName))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("PickContextMenuByComponent key '%s' is listed in PickInteractionExclusions and will never receive pick interaction."),
				*ComponentName.ToString())));
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

		if (!ComponentEntry.Value.PrimaryActionId.IsNone())
		{
			bool bFoundPrimary = false;
			for (const FDIVEPickContextMenuAction& Action : Actions)
			{
				if (DIVEDeviceActionResolve::ResolveActionId(Action) == ComponentEntry.Value.PrimaryActionId)
				{
					bFoundPrimary = true;
					if (!Action.bEnabled)
					{
						Context.AddWarning(FText::FromString(FString::Printf(
							TEXT("PickContextMenuByComponent '%s' PrimaryActionId '%s' points to a disabled action."),
							*ComponentName.ToString(),
							*ComponentEntry.Value.PrimaryActionId.ToString())));
					}
					break;
				}
			}

			if (!bFoundPrimary)
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent '%s' PrimaryActionId '%s' does not match any resolved ActionId in Actions."),
					*ComponentName.ToString(),
					*ComponentEntry.Value.PrimaryActionId.ToString())));
				Result = EDataValidationResult::Invalid;
			}
		}

		for (const FDIVEPickContextMenuAction& Action : Actions)
		{
			const FName ResolvedId = DIVEDeviceActionResolve::ResolveActionId(Action);
			if (ResolvedId.IsNone())
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent '%s' has an entry with unresolved ActionId (set Definition.ActionId or legacy ActionId)."),
					*ComponentName.ToString())));
				Result = EDataValidationResult::Invalid;
				continue;
			}

			if (Action.Definition && !Action.ActionId.IsNone() && Action.ActionId != Action.Definition->ActionId)
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent '%s' row ActionId '%s' ignored; Definition uses '%s'."),
					*ComponentName.ToString(),
					*Action.ActionId.ToString(),
					*Action.Definition->ActionId.ToString())));
			}

			if (DIVE::IsReservedContextMenuActionId(ResolvedId))
			{
				Context.AddError(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent ActionId '%s' is reserved by DIVE built-in menu rows."),
					*ResolvedId.ToString())));
				Result = EDataValidationResult::Invalid;
			}

			if (DIVEDeviceActionResolve::ResolveDisplayName(Action).IsEmpty())
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("PickContextMenuByComponent '%s' action '%s' has an empty resolved DisplayName."),
					*ComponentName.ToString(),
					*ResolvedId.ToString())));
			}
		}
	}

	for (const FName& ExcludedKey : PickInteractionExclusions)
	{
		if (ExcludedKey.IsNone())
		{
			Context.AddWarning(FText::FromString(
				TEXT("PickInteractionExclusions contains an empty key and will never match.")));
			continue;
		}

		FDIVEFocusTarget ResolvedTarget;
		if (!TryResolveStartFocusTarget(ExcludedKey, ResolvedTarget))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("PickInteractionExclusions key '%s' does not match any anchor PartId or pickable mesh component on this actor."),
				*ExcludedKey.ToString())));
		}

		if (PickContextMenuByComponent.Contains(ExcludedKey))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("PickInteractionExclusions key '%s' also has a PickContextMenuByComponent entry; pick interaction will never reach it."),
				*ExcludedKey.ToString())));
		}

		if (PickHoverOverlayByComponent.Contains(ExcludedKey))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("PickInteractionExclusions key '%s' also has a PickHoverOverlayByComponent entry; hover will never apply."),
				*ExcludedKey.ToString())));
		}
	}

	for (const TPair<FName, TSoftObjectPtr<UMaterialInterface>>& OverlayEntry : PickHoverOverlayByComponent)
	{
		const FName ComponentName = OverlayEntry.Key;
		if (ComponentName.IsNone())
		{
			Context.AddWarning(FText::FromString(
				TEXT("PickHoverOverlayByComponent has an entry with an empty key and will never match a pick.")));
			continue;
		}

		FDIVEFocusTarget ResolvedTarget;
		if (!TryResolveStartFocusTarget(ComponentName, ResolvedTarget))
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("PickHoverOverlayByComponent key '%s' does not match any anchor PartId or pickable mesh component on this actor."),
				*ComponentName.ToString())));
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
			const FName ResolvedId = DIVEDeviceActionResolve::ResolveActionId(Action);
			if (ResolvedId.IsNone())
			{
				continue;
			}

			const FName QualifiedActionId = DIVE::MakeQualifiedPickContextMenuActionId(ComponentName, ResolvedId);
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

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	bool bActiveBefore = false;
	const bool bHadActiveBefore = TryQueryPickContextMenuActiveState(
		Owner,
		ComponentName,
		LocalActionId,
		bActiveBefore);

	const bool bHandlerInvoked = TryInvokePickContextMenuHandler(
		Owner,
		ComponentName,
		LocalActionId,
		PickTarget.Primitive.Get(),
		bHadActiveBefore ? &bActiveBefore : nullptr);

	bool bStateToggled = false;
	FName CatalogComponentName = NAME_None;
	const FDIVEPickContextMenuActionList* Catalog = nullptr;
	if (FindPickContextMenuCatalog(PickTarget, CatalogComponentName, Catalog) && Catalog)
	{
		for (const FDIVEPickContextMenuAction& Action : Catalog->Actions)
		{
			if (DIVEDeviceActionResolve::ResolveActionId(Action) != LocalActionId
				|| !DIVEDeviceActionResolve::ResolveToggleActiveSuffix(Action))
			{
				continue;
			}

			bool bActiveAfter = false;
			bStateToggled = TryTogglePickContextMenuActiveState(
				Owner,
				ComponentName,
				LocalActionId,
				bActiveAfter);
			(void)bActiveAfter;
			break;
		}
	}

	return bHandlerInvoked || bStateToggled;
}

bool UDIVEInspectableComponent::FindPickContextMenuCatalog(
	const FDIVEFocusTarget& PickTarget,
	FName& OutComponentName,
	const FDIVEPickContextMenuActionList*& OutCatalog) const
{
	OutComponentName = NAME_None;
	OutCatalog = nullptr;

	if (PickTarget.Kind != EDIVEFocusKind::Primitive || PickContextMenuByComponent.IsEmpty())
	{
		return false;
	}

	auto TryKey = [this, &OutComponentName, &OutCatalog](const FName Key) -> bool
	{
		if (Key.IsNone())
		{
			return false;
		}

		if (const FDIVEPickContextMenuActionList* Found = PickContextMenuByComponent.Find(Key))
		{
			OutComponentName = Key;
			OutCatalog = Found;
			return true;
		}

		return false;
	};

	const UPrimitiveComponent* Primitive = PickTarget.Primitive.Get();
	if (Primitive)
	{
		// 1) Exact Components-panel / instance FName.
		if (TryKey(Primitive->GetFName()))
		{
			return true;
		}

		// 2) Normalized name (Switch2_1 / Switch2_GEN_VARIABLE → Switch2).
		{
			const FString Normalized = DIVE::NormalizeComponentToken(Primitive->GetName());
			if (!Normalized.IsEmpty() && TryKey(FName(*Normalized)))
			{
				return true;
			}
		}

		// 3) Compare normalized tokens both ways (authored key may also carry a suffix).
		{
			const FString PrimitiveToken = DIVE::NormalizeComponentToken(Primitive->GetName());
			for (const TPair<FName, FDIVEPickContextMenuActionList>& Entry : PickContextMenuByComponent)
			{
				if (Entry.Key.IsNone())
				{
					continue;
				}

				if (DIVE::NormalizeComponentToken(Entry.Key.ToString()).Equals(PrimitiveToken, ESearchCase::IgnoreCase))
				{
					return TryKey(Entry.Key);
				}
			}
		}

		// 4) Key resolves to this same pick primitive.
		for (const TPair<FName, FDIVEPickContextMenuActionList>& Entry : PickContextMenuByComponent)
		{
			if (Entry.Key.IsNone())
			{
				continue;
			}

			FDIVEFocusTarget Resolved;
			if (!TryResolveStartFocusTarget(Entry.Key, Resolved)
				|| Resolved.Kind != EDIVEFocusKind::Primitive
				|| Resolved.Primitive.Get() != Primitive)
			{
				continue;
			}

			return TryKey(Entry.Key);
		}
	}

	// 5) Catalog key authored as anchor PartId — only when it does not resolve to a *different* mesh.
	if (!PickTarget.SemanticPartId.IsNone())
	{
		FDIVEFocusTarget ResolvedPart;
		const bool bResolved = TryResolveStartFocusTarget(PickTarget.SemanticPartId, ResolvedPart);
		if (bResolved && ResolvedPart.Kind == EDIVEFocusKind::Anchor)
		{
			if (TryKey(PickTarget.SemanticPartId))
			{
				return true;
			}
		}
		else if (bResolved
			&& ResolvedPart.Kind == EDIVEFocusKind::Primitive
			&& Primitive
			&& ResolvedPart.Primitive.Get() == Primitive)
		{
			if (TryKey(PickTarget.SemanticPartId))
			{
				return true;
			}
		}
		else if (!bResolved)
		{
			// PartId-only key with no live resolve still allowed (same as before for empty registry timing).
			if (TryKey(PickTarget.SemanticPartId))
			{
				return true;
			}
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
	const FDIVEPickContextMenuActionList* Catalog = nullptr;
	if (!FindPickContextMenuCatalog(PickTarget, MatchedComponentName, Catalog) || !Catalog)
	{
		return false;
	}

	for (const FDIVEPickContextMenuAction& Action : Catalog->Actions)
	{
		const FName ResolvedId = DIVEDeviceActionResolve::ResolveActionId(Action);
		if (ResolvedId.IsNone())
		{
			continue;
		}

		if (DIVE::MakeQualifiedPickContextMenuActionId(MatchedComponentName, ResolvedId) == QualifiedActionId)
		{
			OutComponentName = MatchedComponentName;
			OutLocalActionId = ResolvedId;
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
	const FDIVEPickContextMenuActionList* Catalog = nullptr;
	if (!FindPickContextMenuCatalog(PickTarget, ComponentName, Catalog) || !Catalog || Catalog->Actions.IsEmpty())
	{
		return;
	}

	AActor* Owner = GetOwner();
	InOutEntries.Reserve(InOutEntries.Num() + Catalog->Actions.Num());

	for (const FDIVEPickContextMenuAction& Action : Catalog->Actions)
	{
		const FName ResolvedId = DIVEDeviceActionResolve::ResolveActionId(Action);
		if (ResolvedId.IsNone())
		{
			continue;
		}

		FDIVEContextMenuEntry Entry;
		Entry.ActionId = DIVE::MakeQualifiedPickContextMenuActionId(ComponentName, ResolvedId);
		Entry.DisplayName = DIVEDeviceActionResolve::ResolveDisplayName(Action);
		if (DIVEDeviceActionResolve::ResolveToggleActiveSuffix(Action) && Owner)
		{
			bool bActive = false;
			if (TryQueryPickContextMenuActiveState(Owner, ComponentName, ResolvedId, bActive))
			{
				Entry.DisplayName = DIVEContextMenu::FormatActiveLabelSuffix(Entry.DisplayName, bActive);
			}
		}
		Entry.bEnabled = Action.bEnabled;
		InOutEntries.Add(Entry);
	}
}

bool UDIVEInspectableComponent::TryResolvePrimaryPickAction(
	const FDIVEFocusTarget& PickTarget,
	FName& OutQualifiedActionId) const
{
	OutQualifiedActionId = NAME_None;

	FName ComponentName = NAME_None;
	const FDIVEPickContextMenuActionList* Catalog = nullptr;
	if (!FindPickContextMenuCatalog(PickTarget, ComponentName, Catalog) || !Catalog || Catalog->PrimaryActionId.IsNone())
	{
		return false;
	}

	for (const FDIVEPickContextMenuAction& Action : Catalog->Actions)
	{
		const FName ResolvedId = DIVEDeviceActionResolve::ResolveActionId(Action);
		if (ResolvedId == Catalog->PrimaryActionId && Action.bEnabled)
		{
			OutQualifiedActionId = DIVE::MakeQualifiedPickContextMenuActionId(ComponentName, ResolvedId);
			return true;
		}
	}

	return false;
}

bool UDIVEInspectableComponent::TryGetResolvedActionRow(
	const FName CatalogKey,
	const FName ActionId,
	FDIVEResolvedPickAction& OutResolved) const
{
	OutResolved = FDIVEResolvedPickAction();
	if (CatalogKey.IsNone() || ActionId.IsNone())
	{
		return false;
	}

	const FDIVEPickContextMenuActionList* Catalog = PickContextMenuByComponent.Find(CatalogKey);
	if (!Catalog)
	{
		return false;
	}

	for (const FDIVEPickContextMenuAction& Action : Catalog->Actions)
	{
		if (DIVEDeviceActionResolve::ResolveActionId(Action) != ActionId)
		{
			continue;
		}

		OutResolved.CatalogKey = CatalogKey;
		OutResolved.ActionId = ActionId;
		OutResolved.DisplayName = DIVEDeviceActionResolve::ResolveDisplayName(Action);
		OutResolved.bEnabled = Action.bEnabled;
		OutResolved.bToggleActiveSuffix = DIVEDeviceActionResolve::ResolveToggleActiveSuffix(Action);
		OutResolved.UnscrewTurnCount = DIVEDeviceActionResolve::ResolveUnscrewTurnCount(Action);
		OutResolved.Definition = Action.Definition;
		return true;
	}

	return false;
}

bool UDIVEInspectableComponent::FindPickHoverOverlaySoftMaterial(
	const FDIVEFocusTarget& PickTarget,
	TSoftObjectPtr<UMaterialInterface>& OutSoftMaterial) const
{
	OutSoftMaterial.Reset();

	if (PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		return false;
	}

	if (const UPrimitiveComponent* Primitive = PickTarget.Primitive.Get())
	{
		if (const TSoftObjectPtr<UMaterialInterface>* Found = PickHoverOverlayByComponent.Find(Primitive->GetFName()))
		{
			OutSoftMaterial = *Found;
			return true;
		}
	}

	if (!PickTarget.SemanticPartId.IsNone())
	{
		if (const TSoftObjectPtr<UMaterialInterface>* Found = PickHoverOverlayByComponent.Find(PickTarget.SemanticPartId))
		{
			OutSoftMaterial = *Found;
			return true;
		}
	}

	return false;
}

namespace
{
UMaterialInterface* ResolveSoftMaterial(const TSoftObjectPtr<UMaterialInterface>& SoftMaterial)
{
	if (!SoftMaterial.ToSoftObjectPath().IsValid())
	{
		return nullptr;
	}

	if (UMaterialInterface* Loaded = SoftMaterial.Get())
	{
		return Loaded;
	}

	return SoftMaterial.LoadSynchronous();
}
} // namespace

UMaterialInterface* UDIVEInspectableComponent::ResolvePickHoverOverlayMaterial(const FDIVEFocusTarget& PickTarget) const
{
	if (PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		return nullptr;
	}

	if (const UPrimitiveComponent* Primitive = PickTarget.Primitive.Get())
	{
		if (!IsPrimitiveInteractive(Primitive))
		{
			return nullptr;
		}
	}

	TSoftObjectPtr<UMaterialInterface> SoftMaterial;
	if (FindPickHoverOverlaySoftMaterial(PickTarget, SoftMaterial) && SoftMaterial.ToSoftObjectPath().IsValid())
	{
		if (UMaterialInterface* Material = ResolveSoftMaterial(SoftMaterial))
		{
			return Material;
		}
	}

	return ResolveSoftMaterial(DefaultPickHoverOverlayMaterial);
}
