// Copyright (c) 2026. All Rights Reserved.

#include "DIVEInspectableComponent.h"

#include "Actions/DIVEBuiltInActions.h"
#include "DIVEAnchorComponent.h"
#include "DIVEDeviceAction.h"
#include "DIVEDeviceDefinitionAsset.h"
#include "DIVEConvention.h"
#include "DIVEHierarchy.h"
#include "DIVELog.h"
#include "DIVESessionSubsystem.h"
#include "DIVECameraRig.h"
#include "Materials/MaterialInterface.h"
#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "DIVEActionBindingValidation.h"
#include "Misc/DataValidation.h"
#include "UObject/UnrealType.h"
#endif

namespace
{
template <typename TAction>
TAction* GetOrCreateNamedAction(UObject* Outer, const TCHAR* Name)
{
	if (TAction* Existing = FindObject<TAction>(Outer, Name))
	{
		Existing->SetFlags(RF_Public | RF_Transactional);
		return Existing;
	}

	return NewObject<TAction>(Outer, Name, RF_Public | RF_Transactional);
}
}

UDIVEInspectableComponent::UDIVEInspectableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDIVEInspectableComponent::PostInitProperties()
{
	Super::PostInitProperties();
	SeedDefaultBindingsIfNeeded();
}

void UDIVEInspectableComponent::OnComponentCreated()
{
	Super::OnComponentCreated();
	SeedDefaultBindingsIfNeeded();
}

void UDIVEInspectableComponent::OnRegister()
{
	Super::OnRegister();
	InstanceForeignPrivateActions();
}

void UDIVEInspectableComponent::PreSave(FObjectPreSaveContext SaveContext)
{
	InstanceForeignPrivateActions();
	Super::PreSave(SaveContext);
}

void UDIVEInspectableComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	EndActiveInteraction(false);
	Super::EndPlay(EndPlayReason);
}

bool UDIVEInspectableComponent::ShouldSkipDefaultBindingSeed() const
{
	const UPackage* Package = GetOutermost();
	return HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad)
		|| !Package
		|| Package->HasAnyPackageFlags(PKG_CompiledIn);
}

void UDIVEInspectableComponent::SeedDefaultBindingsIfNeeded()
{
	if (ShouldSkipDefaultBindingSeed() || !Bindings.IsEmpty())
	{
		return;
	}

	if (Sections.IsEmpty())
	{
		FDIVEMenuSection StandardSection;
		StandardSection.SectionId = DIVE::kSectionStandard;
		Sections.Add(StandardSection);
	}

	// RF_Public: placed map instances may legally reference these inners on the BP template.
	UDIVEFocusAction* FocusAction =
		GetOrCreateNamedAction<UDIVEFocusAction>(this, DIVE::kSeededFocusAction);
	UDIVEIsolateAction* IsolateAction =
		GetOrCreateNamedAction<UDIVEIsolateAction>(this, DIVE::kSeededIsolateAction);
	if (FocusAction)
	{
		FocusAction->Presentation = EDIVEActionPresentation::Session;
	}
	if (IsolateAction)
	{
		IsolateAction->Presentation = EDIVEActionPresentation::Session;
	}

	FDIVEActionBinding StandardBinding;
	StandardBinding.BindingId = DIVE::kBindingBuiltInStandard;
	StandardBinding.Targets.MatchMode = EDIVETargetMatchMode::AnyPrimitive;
	StandardBinding.SectionId = DIVE::kSectionStandard;
	StandardBinding.Actions.Add(FocusAction);
	StandardBinding.Actions.Add(IsolateAction);
	Bindings.Add(StandardBinding);
	EnsureSeededAdminDefaults();
}

void UDIVEInspectableComponent::EnsureSeededAdminDefaults()
{
	if (ShouldSkipDefaultBindingSeed())
	{
		return;
	}

	auto EnsureAdminSection = [this]()
	{
		for (const FDIVEMenuSection& Section : Sections)
		{
			if (Section.SectionId == DIVE::kSectionAdmin)
			{
				return;
			}
		}

		FDIVEMenuSection AdminSection;
		AdminSection.SectionId = DIVE::kSectionAdmin;
		AdminSection.Header = NSLOCTEXT("DIVE", "SectionAdminHeader", "Admin");
		Sections.Add(AdminSection);
	};

	auto BindingHasActionClass = [](const FDIVEActionBinding& Binding, const UClass* ActionClass) -> bool
	{
		for (const TObjectPtr<UDIVEDeviceAction>& Action : Binding.Actions)
		{
			if (Action && Action->IsA(ActionClass))
			{
				return true;
			}
		}
		return false;
	};

	for (FDIVEActionBinding& Binding : Bindings)
	{
		if (Binding.BindingId != DIVE::kBindingBuiltInAdmin)
		{
			continue;
		}

		EnsureAdminSection();
		if (!BindingHasActionClass(Binding, UDIVESimulatePhysicsAction::StaticClass()))
		{
			Binding.Actions.Add(
				GetOrCreateNamedAction<UDIVESimulatePhysicsAction>(
					this, DIVE::kSeededSimulatePhysicsAction));
		}
		if (!BindingHasActionClass(Binding, UDIVEDeleteMeshAction::StaticClass()))
		{
			Binding.Actions.Add(
				GetOrCreateNamedAction<UDIVEDeleteMeshAction>(
					this, DIVE::kSeededDeleteMeshAction));
		}
		return;
	}

	EnsureAdminSection();

	FDIVEActionBinding AdminBinding;
	AdminBinding.BindingId = DIVE::kBindingBuiltInAdmin;
	AdminBinding.Targets.MatchMode = EDIVETargetMatchMode::AnyPrimitive;
	AdminBinding.SectionId = DIVE::kSectionAdmin;
	AdminBinding.Actions.Add(
		GetOrCreateNamedAction<UDIVESimulatePhysicsAction>(this, DIVE::kSeededSimulatePhysicsAction));
	AdminBinding.Actions.Add(
		GetOrCreateNamedAction<UDIVEDeleteMeshAction>(this, DIVE::kSeededDeleteMeshAction));
	Bindings.Add(AdminBinding);
}

#if WITH_EDITOR
void UDIVEInspectableComponent::AddAdminDefaultBindings()
{
	Modify();
	if (AActor* Owner = GetOwner())
	{
		Owner->Modify();
	}

	EnsureSeededAdminDefaults();

	auto NotifyProperty = [this](const FName PropertyName)
	{
		if (FProperty* Prop = FindFProperty<FProperty>(StaticClass(), PropertyName))
		{
			FPropertyChangedEvent ChangeEvent(Prop, EPropertyChangeType::ValueSet);
			PostEditChangeProperty(ChangeEvent);
		}
	};
	NotifyProperty(GET_MEMBER_NAME_CHECKED(UDIVEInspectableComponent, Bindings));
	NotifyProperty(GET_MEMBER_NAME_CHECKED(UDIVEInspectableComponent, Sections));
}
#endif

void UDIVEInspectableComponent::InstanceForeignPrivateActions()
{
	if (HasAnyFlags(RF_NeedLoad))
	{
		return;
	}

	const UPackage* Package = GetOutermost();
	if (!Package || Package->HasAnyPackageFlags(PKG_CompiledIn))
	{
		return;
	}

	for (FDIVEActionBinding& Binding : Bindings)
	{
		for (TObjectPtr<UDIVEDeviceAction>& Action : Binding.Actions)
		{
			UDIVEDeviceAction* Existing = Action.Get();
			if (!Existing || Existing->IsIn(this))
			{
				continue;
			}

			// Same package: private inners are legal exports. Catalog / CDO: public shared refs.
			if (Existing->GetOutermost() == Package
				|| Existing->HasAnyFlags(RF_Public | RF_ClassDefaultObject))
			{
				continue;
			}

			UDIVEDeviceAction* InstancedAction = DuplicateObject<UDIVEDeviceAction>(Existing, this);
			if (InstancedAction)
			{
				InstancedAction->SetFlags(RF_Public | RF_Transactional);
				Action = InstancedAction;
			}
		}
	}
}

bool UDIVEInspectableComponent::RequestSession()
{
	FDIVESessionParams Params;
	Params.InitialFocusId = DefaultStartFocusId;
	return RequestSessionWithParams(Params);
}

bool UDIVEInspectableComponent::TryRequestSessionFromActionId(FName ActionId)
{
	if (ActionId != DIVE::kActionOpenDIVE)
	{
		return false;
	}

	return RequestSession();
}

bool UDIVEInspectableComponent::RequestSessionWithParams(const FDIVESessionParams& Params)
{
	UWorld* World = GetWorld();
	if (!GetOwner() || !World)
	{
		return false;
	}

	UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>();
	return Subsystem ? Subsystem->TryBeginSession(GetOwner(), this, Params) : false;
}

void UDIVEInspectableComponent::BuildSemanticRegistry() const
{
	SemanticRegistry.Nodes.Reset();

	// Walk the full device hierarchy (owner + all attached child actors) so that anchors placed on
	// child actors of modular devices are registered, matching the coverage of PickAtScreenPosition
	// and TryResolveStartFocusTarget.
	DIVE::ForEachDeviceActor(GetOwner(), [this](AActor* Actor)
	{
		if (!Actor)
		{
			return;
		}

		TArray<UDIVEAnchorComponent*> Anchors;
		Actor->GetComponents<UDIVEAnchorComponent>(Anchors);
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
	});
}

TArray<FName> UDIVEInspectableComponent::GetAvailableSectionIds() const
{
	TArray<FName> Result = {DIVE::kSectionStandard, DIVE::kSectionAdmin};
	TArray<FDIVEMenuSection> AllSections;
	GatherAuthoredSections(AllSections);
	for (const FDIVEMenuSection& Section : AllSections)
	{
		if (!Section.SectionId.IsNone())
		{
			Result.AddUnique(Section.SectionId);
		}
	}
	return Result;
}

TArray<FName> UDIVEInspectableComponent::GetAvailableBindingIds() const
{
	TArray<FName> Result;
	TArray<const FDIVEActionBinding*> AuthoredBindings;
	GatherAuthoredBindings(AuthoredBindings);
	for (const FDIVEActionBinding* Binding : AuthoredBindings)
	{
		if (Binding && !Binding->BindingId.IsNone())
		{
			Result.AddUnique(Binding->BindingId);
		}
	}
	return Result;
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

	// A primitive hidden via SetHiddenInGame (e.g. by isolation) is logically invisible even when
	// IsVisible() still returns true (IsVisible reflects the render-thread flag, not bHiddenInGame).
	const bool bVisible = Primitive->IsVisible() && !Primitive->bHiddenInGame;
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

bool UDIVEInspectableComponent::HasPickHoverOverlay() const
{
	if (DefaultPickHoverOverlayMaterial.ToSoftObjectPath().IsValid())
	{
		return true;
	}

	for (const TPair<FName, TSoftObjectPtr<UMaterialInterface>>& Pair : PickHoverOverlayByComponent)
	{
		if (Pair.Value.ToSoftObjectPath().IsValid())
		{
			return true;
		}
	}

	return false;
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

FDIVECameraSettings UDIVEInspectableComponent::GetEffectiveCameraSettings() const
{
	if (bUseDeviceDefinitionSettings && DeviceDefinition)
	{
		return DeviceDefinition->CameraSettings;
	}

	return CameraSettings;
}

float UDIVEInspectableComponent::ComputeOrbitDistanceForFocus(const FDIVEFocusTarget& Target) const
{
	const FDIVECameraSettings Settings = GetEffectiveCameraSettings();
	const float MinDistance = Settings.MinOrbitDistanceCm;
	const float MaxDistance = FMath::Max(Settings.MaxOrbitDistanceCm, MinDistance);

	if (Target.Kind == EDIVEFocusKind::Primitive)
	{
		if (const UPrimitiveComponent* Primitive = Target.Primitive.Get())
		{
			const float Radius = FMath::Max(Primitive->Bounds.SphereRadius, 1.f);
			return FMath::Clamp(Radius * Settings.FocusOrbitFitMultiplier, MinDistance, MaxDistance);
		}
	}

	return FMath::Clamp(Settings.DefaultOrbitDistance, MinDistance, MaxDistance);
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
	return Radius * GetEffectiveCameraSettings().FocusNearPaddingFactor;
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
			if (!Primitive || !MatchesComponentNameValue(Primitive, FocusObjectId))
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

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}

	bool bResolved = false;
	DIVE::ForEachDeviceActor(Owner, [PartId, &OutNode, &bResolved](AActor* Actor)
	{
		if (bResolved || !Actor)
		{
			return;
		}

		TArray<UDIVEAnchorComponent*> Anchors;
		Actor->GetComponents<UDIVEAnchorComponent>(Anchors);
		for (UDIVEAnchorComponent* Anchor : Anchors)
		{
			if (!Anchor)
			{
				continue;
			}

			const FName ResolvedPartId = Anchor->GetResolvedPartId();
			if (ResolvedPartId != PartId && Anchor->GetFName() != PartId)
			{
				continue;
			}

			OutNode.PartId = ResolvedPartId;
			OutNode.DisplayName = Anchor->DisplayName.IsEmpty()
				? FText::FromName(ResolvedPartId)
				: Anchor->DisplayName;
			OutNode.SceneComponent = Anchor;
			bResolved = true;
			return;
		}
	});

	return bResolved;
}

bool UDIVEInspectableComponent::IsSessionActive() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			return Subsystem->IsSessionActive() && Subsystem->GetActiveInspectable() == this;
		}
	}

	return false;
}

void UDIVEInspectableComponent::NotifySessionLifecycle(bool bActive)
{
	OnSessionLifecycle.Broadcast(bActive);
}

void UDIVEInspectableComponent::NotifyActionExecuted(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context)
{
	if (!Action)
	{
		return;
	}

	OnActionExecuted.Broadcast(Action, Context);
	Action->OnExecuted.Broadcast(Action, Context);
}

void UDIVEInspectableComponent::NotifyActionValueChanged(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context,
	const FDIVEInteractionValue& Value)
{
	if (!Action)
	{
		return;
	}

	OnActionValueChanged.Broadcast(Action, Context, Value);
}

void UDIVEInspectableComponent::GatherAuthoredBindings(TArray<const FDIVEActionBinding*>& OutBindings) const
{
	OutBindings.Reset();
	for (const FDIVEActionBinding& Binding : Bindings)
	{
		OutBindings.Add(&Binding);
	}

	if (ActionCatalog)
	{
		for (const FDIVEActionBinding& Binding : ActionCatalog->Bindings)
		{
			OutBindings.Add(&Binding);
		}
	}
}

void UDIVEInspectableComponent::GatherAuthoredSections(TArray<FDIVEMenuSection>& OutSections) const
{
	OutSections.Reset();
	TSet<FName> SeenIds;

	auto AddSection = [&OutSections, &SeenIds](const FDIVEMenuSection& Section)
	{
		if (Section.SectionId.IsNone())
		{
			return;
		}

		if (SeenIds.Contains(Section.SectionId))
		{
			for (FDIVEMenuSection& Existing : OutSections)
			{
				if (Existing.SectionId == Section.SectionId && Existing.Header.IsEmpty() && !Section.Header.IsEmpty())
				{
					Existing.Header = Section.Header;
					break;
				}
			}
			return;
		}

		SeenIds.Add(Section.SectionId);
		OutSections.Add(Section);
	};

	for (const FDIVEMenuSection& Section : Sections)
	{
		AddSection(Section);
	}

	if (ActionCatalog)
	{
		for (const FDIVEMenuSection& Section : ActionCatalog->Sections)
		{
			AddSection(Section);
		}
	}
}

bool UDIVEInspectableComponent::MatchesComponentNameValue(
	const UPrimitiveComponent* Primitive,
	const FName MatchValue) const
{
	// Name-only: exact FName, then BP-stable token. Do not call TryResolveStartFocusTarget.
	if (!Primitive || MatchValue.IsNone())
	{
		return false;
	}

	if (Primitive->GetFName() == MatchValue)
	{
		return true;
	}

	const FString PrimitiveToken = DIVE::NormalizeComponentToken(Primitive->GetName());
	const FString MatchToken = DIVE::NormalizeComponentToken(MatchValue.ToString());
	if (!PrimitiveToken.IsEmpty()
		&& !MatchToken.IsEmpty()
		&& PrimitiveToken.Equals(MatchToken, ESearchCase::IgnoreCase))
	{
		return true;
	}

	return false;
}

bool UDIVEInspectableComponent::DoesTargetQueryMatchPick(
	const FDIVETargetQuery& Query,
	const FDIVEFocusTarget& PickTarget) const
{
	if (PickTarget.Kind != EDIVEFocusKind::Primitive)
	{
		return false;
	}

	const UPrimitiveComponent* Primitive = PickTarget.Primitive.Get();
	if (!Primitive)
	{
		return false;
	}

	if (Query.MatchMode == EDIVETargetMatchMode::AnyPrimitive)
	{
		return true;
	}

	if (Query.MatchValues.IsEmpty())
	{
		return false;
	}

	switch (Query.MatchMode)
	{
	case EDIVETargetMatchMode::ComponentTag:
		for (const FName MatchTag : Query.MatchValues)
		{
			if (!MatchTag.IsNone() && Primitive->ComponentHasTag(MatchTag))
			{
				return true;
			}
		}
		return false;

	case EDIVETargetMatchMode::ComponentName:
		for (const FName MatchValue : Query.MatchValues)
		{
			if (MatchesComponentNameValue(Primitive, MatchValue))
			{
				return true;
			}
		}
		return false;

	case EDIVETargetMatchMode::PartId:
		if (PickTarget.SemanticPartId.IsNone())
		{
			return false;
		}
		for (const FName MatchValue : Query.MatchValues)
		{
			if (!MatchValue.IsNone() && MatchValue == PickTarget.SemanticPartId)
			{
				return true;
			}
		}
		return false;

	default:
		return false;
	}
}

FName UDIVEInspectableComponent::ResolveTargetKeyForQuery(
	const FDIVETargetQuery& Query,
	const FDIVEFocusTarget& PickTarget) const
{
	const UPrimitiveComponent* Primitive = PickTarget.Primitive.Get();

	switch (Query.MatchMode)
	{
	case EDIVETargetMatchMode::ComponentName:
		if (Primitive)
		{
			for (const FName MatchValue : Query.MatchValues)
			{
				if (MatchesComponentNameValue(Primitive, MatchValue))
				{
					return MatchValue;
				}
			}
		}
		break;

	case EDIVETargetMatchMode::PartId:
		if (!PickTarget.SemanticPartId.IsNone())
		{
			return PickTarget.SemanticPartId;
		}
		break;

	case EDIVETargetMatchMode::ComponentTag:
	default:
		break;
	}

	return Primitive ? Primitive->GetFName() : PickTarget.SemanticPartId;
}

void UDIVEInspectableComponent::CollectPrimitivesMatchingQuery(
	const FDIVETargetQuery& Query,
	TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	OutPrimitives.Reset();

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	BuildSemanticRegistry();

	TArray<UPrimitiveComponent*> DevicePrimitives;
	DIVE::CollectDevicePrimitives(Owner, DevicePrimitives);
	for (UPrimitiveComponent* Primitive : DevicePrimitives)
	{
		if (!Primitive || !IsPrimitiveInteractive(Primitive))
		{
			continue;
		}

		const FDIVEFocusTarget PickTarget =
			FDIVEFocusTarget::FromPrimitive(Primitive, ResolveSemanticPartId(Primitive));
		if (DoesTargetQueryMatchPick(Query, PickTarget))
		{
			OutPrimitives.Add(Primitive);
		}
	}
}

void UDIVEInspectableComponent::GatherMatchingBindings(
	const FDIVEFocusTarget& PickTarget,
	TArray<const FDIVEActionBinding*>& OutBindings) const
{
	OutBindings.Reset();
	TArray<const FDIVEActionBinding*> AllBindings;
	GatherAuthoredBindings(AllBindings);

	// Authored order only (component Bindings, then catalog). Menu row order follows this list.
	for (const FDIVEActionBinding* Binding : AllBindings)
	{
		if (Binding && DoesTargetQueryMatchPick(Binding->Targets, PickTarget))
		{
			OutBindings.Add(Binding);
		}
	}
}

TArray<UDIVEDeviceAction*> UDIVEInspectableComponent::GetActionInstances(FName BindingId) const
{
	TArray<UDIVEDeviceAction*> Result;
	if (BindingId.IsNone())
	{
		return Result;
	}

	TArray<const FDIVEActionBinding*> AuthoredBindings;
	GatherAuthoredBindings(AuthoredBindings);
	for (const FDIVEActionBinding* Binding : AuthoredBindings)
	{
		if (!Binding || Binding->BindingId != BindingId)
		{
			continue;
		}

		for (UDIVEDeviceAction* Action : Binding->Actions)
		{
			if (Action)
			{
				Result.Add(Action);
			}
		}
	}

	return Result;
}

UDIVEDeviceAction* UDIVEInspectableComponent::FindActionInstance(
	TSubclassOf<UDIVEDeviceAction> ActionClass,
	FName BindingId) const
{
	if (!ActionClass)
	{
		return nullptr;
	}

	TArray<const FDIVEActionBinding*> AuthoredBindings;
	GatherAuthoredBindings(AuthoredBindings);
	for (const FDIVEActionBinding* Binding : AuthoredBindings)
	{
		if (!Binding)
		{
			continue;
		}

		if (!BindingId.IsNone() && Binding->BindingId != BindingId)
		{
			continue;
		}

		for (UDIVEDeviceAction* Action : Binding->Actions)
		{
			if (Action && Action->IsA(ActionClass))
			{
				return Action;
			}
		}
	}

	return nullptr;
}

bool UDIVEInspectableComponent::TryResolvePrimaryAction(
	const FDIVEFocusTarget& PickTarget,
	UDIVEDeviceAction*& OutAction,
	FName& OutTargetKey,
	FName& OutBindingId) const
{
	OutAction = nullptr;
	OutTargetKey = NAME_None;
	OutBindingId = NAME_None;

	TArray<const FDIVEActionBinding*> Matched;
	GatherMatchingBindings(PickTarget, Matched);

	const FDIVEActionBinding* Winner = SelectPrimaryBinding(Matched);
	if (!Winner)
	{
		return false;
	}

	OutAction = Winner->GetPrimaryAction();
	OutTargetKey = ResolveTargetKeyForQuery(Winner->Targets, PickTarget);
	OutBindingId = Winner->BindingId;
	return OutAction != nullptr;
}

FDIVEActionContext UDIVEInspectableComponent::MakeActionContext(
	const FDIVEFocusTarget& PickTarget,
	FName TargetKey,
	const FVector2D& ScreenPosition,
	const FHitResult& PickHit,
	FName BindingId,
	const bool bOverrideView,
	const FVector ViewLocation,
	const FRotator ViewRotation) const
{
	FDIVEActionContext Context;
	Context.DeviceHost = GetOwner();
	Context.SourceComponent = const_cast<UDIVEInspectableComponent*>(this);
	Context.Target = PickTarget.Primitive.Get();
	Context.TargetKey = TargetKey.IsNone()
		? (Context.Target ? Context.Target->GetFName() : PickTarget.SemanticPartId)
		: TargetKey;
	Context.BindingId = BindingId;
	Context.PickTarget = PickTarget;
	Context.ScreenPosition = ScreenPosition;
	Context.PickHit = PickHit;

	if (bOverrideView)
	{
		Context.ViewLocation = ViewLocation;
		Context.ViewRotation = ViewRotation;
	}
	else if (UWorld* World = GetWorld())
	{
		if (const UDIVESessionSubsystem* Session = World->GetSubsystem<UDIVESessionSubsystem>())
		{
			if (const ADIVECameraRig* CameraRig = Session->GetActiveCameraRig())
			{
				Context.ViewLocation = CameraRig->GetActorLocation();
				Context.ViewRotation = CameraRig->GetActorRotation();
			}
		}
	}

	if (PickHit.bBlockingHit || !PickHit.TraceStart.Equals(PickHit.TraceEnd))
	{
		Context.PickRayDir = (PickHit.TraceEnd - PickHit.TraceStart).GetSafeNormal();
	}
	if (Context.PickRayDir.IsNearlyZero())
	{
		Context.PickRayDir = Context.ViewRotation.Vector();
	}

	return Context;
}

bool UDIVEInspectableComponent::ExecuteAction(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context)
{
	return DIVEActionExecution::ExecuteResolvedAction(
		GetWorld(),
		this,
		ContinuousSlot,
		Action,
		Context);
}

void UDIVEInspectableComponent::UpdateActiveInteraction(const FDIVEInteractionUpdate& Update)
{
	if (!ContinuousSlot.ActiveAction.IsValid())
	{
		ContinuousSlot.Reset();
		return;
	}

	if (!ContinuousSlot.IsActive())
	{
		EndActiveInteraction(true);
		return;
	}

	DIVEActionExecution::UpdateContinuousAction(GetWorld(), ContinuousSlot, Update);
	if (ContinuousSlot.ActiveAction.IsValid() && !ContinuousSlot.ActiveAction->IsInteractionActive())
	{
		EndActiveInteraction(true);
	}
}

void UDIVEInspectableComponent::EndActiveInteraction(const bool bCommit)
{
	if (!ContinuousSlot.ActiveAction.IsValid() && !ContinuousSlot.IsActive())
	{
		ContinuousSlot.Reset();
		return;
	}

	DIVEActionExecution::EndContinuousAction(GetWorld(), this, ContinuousSlot, bCommit);
}

bool UDIVEInspectableComponent::HasActiveInteraction() const
{
	return ContinuousSlot.IsActive();
}

void UDIVEInspectableComponent::HandleContinuousActionValueChanged(
	UDIVEDeviceAction* Action,
	const FDIVEActionContext& Context,
	const FDIVEInteractionValue& Value)
{
	NotifyActionValueChanged(Action, Context, Value);
}

void UDIVEInspectableComponent::AppendConfiguredContextMenuEntries(
	const FDIVEFocusTarget& PickTarget,
	TArray<FDIVEContextMenuEntry>& InOutEntries,
	const EDIVEActionPresentation PresentationFilter) const
{
	TArray<const FDIVEActionBinding*> Matched;
	GatherMatchingBindings(PickTarget, Matched);
	if (Matched.IsEmpty())
	{
		return;
	}

	TArray<FDIVEMenuSection> AuthoredSections;
	GatherAuthoredSections(AuthoredSections);

	struct FPendingRow
	{
		UDIVEDeviceAction* Action = nullptr;
		FText DisplayName;
		bool bEnabled = true;
		bool bChecked = false;
		FName TargetKey = NAME_None;
		FName BindingId = NAME_None;
	};

	TMap<FName, TArray<FPendingRow>> RowsBySection;

	for (const FDIVEActionBinding* Binding : Matched)
	{
		if (!Binding)
		{
			continue;
		}

		const FName SectionId = Binding->SectionId.IsNone() ? DIVE::kSectionStandard : Binding->SectionId;
		const FName TargetKey = ResolveTargetKeyForQuery(Binding->Targets, PickTarget);
		const FDIVEActionContext Context = MakeActionContext(
			PickTarget,
			TargetKey,
			FVector2D::ZeroVector,
			FHitResult(),
			Binding->BindingId);
		UWorld* ContextWorld = GetOwner() ? GetOwner()->GetWorld() : nullptr;

		for (UDIVEDeviceAction* Action : Binding->Actions)
		{
			if (!Action)
			{
				continue;
			}

			if (!ActionMatchesPresentationFilter(Action->Presentation, PresentationFilter))
			{
				continue;
			}

			FDIVEActionWorldScope WorldScope(Action, ContextWorld);
			const FDIVEActionDisplayState Display = Action->GetDisplayState(Context);
			if (!Display.bVisible)
			{
				continue;
			}

			FPendingRow Row;
			Row.Action = Action;
			Row.DisplayName = Display.DisplayName;
			Row.bEnabled = Display.bEnabled;
			Row.bChecked = Display.bChecked;
			Row.TargetKey = TargetKey;
			Row.BindingId = Binding->BindingId;
			RowsBySection.FindOrAdd(SectionId).Add(Row);
		}
	}

	auto AppendSectionRows = [&](FName SectionId)
	{
		TArray<FPendingRow>* Rows = RowsBySection.Find(SectionId);
		if (!Rows || Rows->IsEmpty())
		{
			return;
		}

		TSet<FString> SeenLabels;
		for (const FPendingRow& Row : *Rows)
		{
			const FString LabelKey = Row.DisplayName.ToString();
			if (SeenLabels.Contains(LabelKey))
			{
				UE_LOG(LogDIVE, Warning,
					TEXT("Duplicate context-menu DisplayName '%s' in section '%s' on '%s'."),
					*LabelKey,
					*SectionId.ToString(),
					*GetNameSafe(GetOwner()));
			}
			else
			{
				SeenLabels.Add(LabelKey);
			}

			FDIVEContextMenuEntry Entry;
			Entry.Action = Row.Action;
			Entry.DisplayName = Row.DisplayName;
			Entry.bEnabled = Row.bEnabled;
			Entry.bChecked = Row.bChecked;
			Entry.TargetKey = Row.TargetKey;
			Entry.BindingId = Row.BindingId;
			Entry.bIsSeparator = false;
			InOutEntries.Add(Entry);
		}

		RowsBySection.Remove(SectionId);
	};

	TArray<FName> SectionOrder;
	TMap<FName, FText> HeaderBySection;
	for (const FDIVEMenuSection& Section : AuthoredSections)
	{
		SectionOrder.Add(Section.SectionId);
		if (!Section.Header.IsEmpty())
		{
			HeaderBySection.Add(Section.SectionId, Section.Header);
		}
	}
	for (const FDIVEActionBinding* Binding : Matched)
	{
		if (!Binding)
		{
			continue;
		}

		const FName SectionId = Binding->SectionId.IsNone() ? DIVE::kSectionStandard : Binding->SectionId;
		SectionOrder.AddUnique(SectionId);
	}

	for (const FName SectionId : SectionOrder)
	{
		if (!RowsBySection.Contains(SectionId) || RowsBySection[SectionId].IsEmpty())
		{
			continue;
		}

		const FText* Header = HeaderBySection.Find(SectionId);
		const bool bHasHeader = Header && !Header->IsEmpty();
		const bool bNeedDividerBefore = !InOutEntries.IsEmpty() && !InOutEntries.Last().bIsSeparator;
		if (bNeedDividerBefore || (InOutEntries.IsEmpty() && bHasHeader))
		{
			FDIVEContextMenuEntry Separator;
			Separator.bIsSeparator = true;
			if (bHasHeader)
			{
				Separator.DisplayName = *Header;
			}
			InOutEntries.Add(Separator);
		}

		AppendSectionRows(SectionId);
	}
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

void UDIVEInspectableComponent::PreloadPickHoverOverlays()
{
	ResolveSoftMaterial(DefaultPickHoverOverlayMaterial);
	for (const TPair<FName, TSoftObjectPtr<UMaterialInterface>>& Pair : PickHoverOverlayByComponent)
	{
		ResolveSoftMaterial(Pair.Value);
	}
}

#if WITH_EDITOR

namespace
{
FString MakeBindingValidationLabel(const FDIVEActionBinding* Binding, const int32 BindingIndex)
{
	if (Binding && !Binding->BindingId.IsNone())
	{
		return Binding->BindingId.ToString();
	}

	return FString::FromInt(BindingIndex);
}
}

void UDIVEInspectableComponent::AppendDeviceAuthoringValidation(FDataValidationContext& Context) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	BuildSemanticRegistry();

	TArray<UPrimitiveComponent*> DevicePrimitives;
	DIVE::CollectDevicePrimitives(Owner, DevicePrimitives);

	TArray<const FDIVEActionBinding*> AllBindings;
	GatherAuthoredBindings(AllBindings);

	// Equal-specificity primary overlap on the same interactive primitive.
	for (UPrimitiveComponent* Primitive : DevicePrimitives)
	{
		if (!Primitive || !IsPrimitiveInteractive(Primitive))
		{
			continue;
		}

		const FName SemanticPartId = ResolveSemanticPartId(Primitive);
		const FDIVEFocusTarget PickTarget = FDIVEFocusTarget::FromPrimitive(Primitive, SemanticPartId);

		TArray<const FDIVEActionBinding*> PrimaryCandidates;
		int32 MaxSpecificity = INDEX_NONE;
		for (const FDIVEActionBinding* Binding : AllBindings)
		{
			if (!Binding
				|| !Binding->GetPrimaryAction()
				|| !DoesTargetQueryMatchPick(Binding->Targets, PickTarget))
			{
				continue;
			}

			const int32 Specificity = GetTargetMatchSpecificity(Binding->Targets.MatchMode);
			if (PrimaryCandidates.IsEmpty() || Specificity > MaxSpecificity)
			{
				PrimaryCandidates.Reset();
				PrimaryCandidates.Add(Binding);
				MaxSpecificity = Specificity;
			}
			else if (Specificity == MaxSpecificity)
			{
				PrimaryCandidates.Add(Binding);
			}
		}

		if (PrimaryCandidates.Num() > 1)
		{
			TArray<FString> Labels;
			for (const FDIVEActionBinding* Binding : PrimaryCandidates)
			{
				Labels.Add(Binding && !Binding->BindingId.IsNone()
					? Binding->BindingId.ToString()
					: TEXT("<unnamed>"));
			}

			const FString WinnerLabel = Labels.IsEmpty() ? TEXT("<unnamed>") : Labels[0];
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("Primitive '%s' has %d primary bindings at equal match specificity (%d): %s. Earlier binding '%s' wins LMB; clear PrimaryActionIndex on the others if unintended."),
				*Primitive->GetName(),
				PrimaryCandidates.Num(),
				MaxSpecificity,
				*FString::Join(Labels, TEXT(", ")),
				*WinnerLabel)));
		}
	}

	for (int32 BindingIndex = 0; BindingIndex < AllBindings.Num(); ++BindingIndex)
	{
		const FDIVEActionBinding* Binding = AllBindings[BindingIndex];
		if (!Binding)
		{
			continue;
		}

		bool bHasKinematicDrive = false;
		for (const UDIVEDeviceAction* Action : Binding->Actions)
		{
			if (Action && (Action->IsA<UDIVERotaryDriveAction>()
				|| Action->IsA<UDIVEThreadedDriveAction>()
				|| Action->IsA<UDIVELinearDriveAction>()))
			{
				bHasKinematicDrive = true;
				break;
			}
		}
		if (!bHasKinematicDrive)
		{
			continue;
		}

		TArray<UPrimitiveComponent*> Matching;
		CollectPrimitivesMatchingQuery(Binding->Targets, Matching);
		for (const UPrimitiveComponent* Primitive : Matching)
		{
			if (Primitive && Primitive->IsSimulatingPhysics())
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("Binding '%s': '%s' has Simulate Physics on. Rotate/Unscrew/Slide Begin will turn it off for the kinematic drag."),
					*MakeBindingValidationLabel(Binding, BindingIndex),
					*Primitive->GetName())));
			}
		}
	}

	for (int32 BindingIndex = 0; BindingIndex < AllBindings.Num(); ++BindingIndex)
	{
		const FDIVEActionBinding* Binding = AllBindings[BindingIndex];
		if (!Binding || Binding->Targets.MatchMode != EDIVETargetMatchMode::ComponentTag)
		{
			continue;
		}

		const FString BindingLabel = MakeBindingValidationLabel(Binding, BindingIndex);

		bool bAnyTagged = false;
		for (const FName MatchTag : Binding->Targets.MatchValues)
		{
			if (MatchTag.IsNone())
			{
				continue;
			}
			for (const UPrimitiveComponent* Primitive : DevicePrimitives)
			{
				if (Primitive && Primitive->ComponentHasTag(MatchTag))
				{
					bAnyTagged = true;
					break;
				}
			}
			if (bAnyTagged)
			{
				break;
			}
		}
		if (!bAnyTagged && !Binding->Targets.MatchValues.IsEmpty())
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("Binding '%s' ComponentTag MatchValues match no device component tags."),
				*BindingLabel)));
		}
	}

	// PartId bindings: each MatchValue must resolve to an anchor that has attached primitives.
	TArray<UDIVEAnchorComponent*> Anchors;
	DIVE::CollectDeviceComponents<UDIVEAnchorComponent>(Owner, Anchors);

	for (int32 BindingIndex = 0; BindingIndex < AllBindings.Num(); ++BindingIndex)
	{
		const FDIVEActionBinding* Binding = AllBindings[BindingIndex];
		if (!Binding || Binding->Targets.MatchMode != EDIVETargetMatchMode::PartId)
		{
			continue;
		}

		const FString BindingLabel = MakeBindingValidationLabel(Binding, BindingIndex);
		for (const FName MatchPartId : Binding->Targets.MatchValues)
		{
			if (MatchPartId.IsNone())
			{
				continue;
			}

			UDIVEAnchorComponent* MatchedAnchor = nullptr;
			for (UDIVEAnchorComponent* Anchor : Anchors)
			{
				if (Anchor && Anchor->GetResolvedPartId() == MatchPartId)
				{
					MatchedAnchor = Anchor;
					break;
				}
			}

			if (!MatchedAnchor)
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("Binding '%s' PartId '%s' matches no DIVE Anchor on this device (PartId is the anchor field, not a component name)."),
					*BindingLabel,
					*MatchPartId.ToString())));
				continue;
			}

			TArray<UPrimitiveComponent*> AttachedPrimitives;
			DIVE::CollectAttachedPrimitives(MatchedAnchor, AttachedPrimitives);
			// Anchor itself is a SceneComponent — CollectAttachedPrimitives only adds UPrimitiveComponent
			// children (and the root if it were a primitive). Empty = nothing to pick under this PartId.
			if (AttachedPrimitives.IsEmpty())
			{
				Context.AddWarning(FText::FromString(FString::Printf(
					TEXT("Binding '%s' PartId '%s' has anchor '%s' but no attached primitives — attach Box/mesh under the anchor."),
					*BindingLabel,
					*MatchPartId.ToString(),
					*GetNameSafe(MatchedAnchor))));
			}
		}
	}

	// Shape pick volumes must Block the pick channel (Trigger profiles often Ignore Visibility).
	for (const UPrimitiveComponent* Primitive : DevicePrimitives)
	{
		if (!Primitive || !Primitive->IsA(UShapeComponent::StaticClass()))
		{
			continue;
		}

		if (!SkipComponentTag.IsNone() && Primitive->ComponentHasTag(SkipComponentTag))
		{
			continue;
		}

		const ECollisionResponse Response = Primitive->GetCollisionResponseToChannel(PickTraceChannel);
		if (Response != ECR_Block)
		{
			Context.AddWarning(FText::FromString(FString::Printf(
				TEXT("Shape '%s' does not Block pick channel %d (response=%d). Set Collision Responses so DIVE's Pick Trace Channel Blocks (default Visibility)."),
				*Primitive->GetName(),
				static_cast<int32>(PickTraceChannel.GetValue()),
				static_cast<int32>(Response))));
		}
	}

}

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
		if (CameraSettings.OrbitSensitivity <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("OrbitSensitivity must be greater than zero.")));
			Result = EDataValidationResult::Invalid;
		}

		if (CameraSettings.ZoomSensitivity <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("ZoomSensitivity must be greater than zero.")));
			Result = EDataValidationResult::Invalid;
		}

		if (CameraSettings.DefaultOrbitDistance <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("DefaultOrbitDistance must be greater than zero.")));
			Result = EDataValidationResult::Invalid;
		}

		if (CameraSettings.MinOrbitDistanceCm <= 0.f
			|| CameraSettings.MaxOrbitDistanceCm < CameraSettings.MinOrbitDistanceCm)
		{
			Context.AddError(FText::FromString(TEXT("Orbit distance limits must satisfy 0 < Min <= Max.")));
			Result = EDataValidationResult::Invalid;
		}

		if (CameraSettings.bScaleZoomWithOrbitDistance && CameraSettings.ZoomDistanceReferenceCm <= 0.f)
		{
			Context.AddError(FText::FromString(TEXT("ZoomDistanceReferenceCm must be greater than zero.")));
			Result = EDataValidationResult::Invalid;
		}

		if (CameraSettings.FocusBlendDuration < 0.f)
		{
			Context.AddError(FText::FromString(TEXT("FocusBlendDuration must be greater than or equal to zero.")));
			Result = EDataValidationResult::Invalid;
		}
	}
	else if (!DeviceDefinition)
	{
		Context.AddWarning(FText::FromString(
			TEXT("bUseDeviceDefinitionSettings is enabled but DeviceDefinition is not assigned.")));
	}

	TArray<UDIVEAnchorComponent*> Anchors;
	DIVE::CollectDeviceComponents<UDIVEAnchorComponent>(Owner, Anchors);

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

	TArray<FDIVEMenuSection> AllSections;
	GatherAuthoredSections(AllSections);
	TSet<FName> KnownSectionIds;
	if (!DIVEActionBindingValidation::ValidateSections(AllSections, KnownSectionIds, Context))
	{
		Result = EDataValidationResult::Invalid;
	}

	TArray<const FDIVEActionBinding*> AllBindings;
	GatherAuthoredBindings(AllBindings);
	if (!DIVEActionBindingValidation::ValidateBindings(AllBindings, KnownSectionIds, Context))
	{
		Result = EDataValidationResult::Invalid;
	}

	AppendDeviceAuthoringValidation(Context);

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

	return Result;
}

#endif // WITH_EDITOR
