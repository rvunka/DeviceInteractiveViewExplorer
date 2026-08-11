// Copyright (c) 2026. All Rights Reserved.

#include "K2Nodes/K2Node_DIVEActionEvent.h"

#include "K2Nodes/K2Node_DIVEActionBoundEvent.h"

#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintNodeTemplateCache.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "K2Node_DynamicCast.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_DIVEActionEvent)

#define LOCTEXT_NAMESPACE "K2Node_DIVEActionEvent"

namespace
{
const FName ActionPinName(TEXT("Action"));
const FName ContextPinName(TEXT("Context"));

bool IsUsableActionClass(const UClass* Class)
{
	if (!Class
		|| !Class->IsChildOf(UDIVEDeviceAction::StaticClass())
		|| Class == UDIVEDeviceAction::StaticClass()
		|| Class == UDIVEContinuousDeviceAction::StaticClass())
	{
		return false;
	}

	if (Class->HasAnyClassFlags(
			CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Hidden))
	{
		return false;
	}

	const FString Name = Class->GetName();
	if (Name.StartsWith(TEXT("SKEL_")) || Name.StartsWith(TEXT("REINST_")) || Name.StartsWith(TEXT("TRASHCLASS_")))
	{
		return false;
	}

	return true;
}

bool BlueprintHasInspectableComponent(const UBlueprint* Blueprint)
{
	if (!Blueprint)
	{
		return false;
	}

	auto HasInspectableInClass = [](const UClass* Class) -> bool
	{
		if (!Class)
		{
			return false;
		}

		for (TFieldIterator<FObjectProperty> PropIt(Class); PropIt; ++PropIt)
		{
			const FObjectProperty* ObjProp = *PropIt;
			if (ObjProp && ObjProp->PropertyClass && ObjProp->PropertyClass->IsChildOf(UDIVEInspectableComponent::StaticClass()))
			{
				return true;
			}
		}
		return false;
	};

	if (HasInspectableInClass(Blueprint->SkeletonGeneratedClass) || HasInspectableInClass(Blueprint->GeneratedClass))
	{
		return true;
	}

	if (Blueprint->SimpleConstructionScript)
	{
		for (const USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->ComponentClass && Node->ComponentClass->IsChildOf(UDIVEInspectableComponent::StaticClass()))
			{
				return true;
			}
		}
	}

	return false;
}

void CollectActionClasses(TArray<UClass*>& OutClasses)
{
	OutClasses.Reset();

	// Loaded classes (native + already-loaded Blueprint generated classes).
	TArray<UClass*> Derived;
	GetDerivedClasses(UDIVEDeviceAction::StaticClass(), Derived, /*bRecursive=*/true);
	for (UClass* Class : Derived)
	{
		if (IsUsableActionClass(Class))
		{
			OutClasses.AddUnique(Class);
		}
	}

	// Unloaded Blueprint subclasses via Asset Registry (plan: AR + GetDerivedClasses).
	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TArray<FTopLevelAssetPath> BaseClasses;
	BaseClasses.Add(UDIVEDeviceAction::StaticClass()->GetClassPathName());

	TSet<FTopLevelAssetPath> Excluded;
	TSet<FTopLevelAssetPath> DerivedPaths;
	AssetRegistry.GetDerivedClassNames(BaseClasses, Excluded, DerivedPaths);

	for (const FTopLevelAssetPath& ClassPath : DerivedPaths)
	{
		const FString PathString = ClassPath.ToString();
		UClass* Class = FindObject<UClass>(nullptr, *PathString);
		if (!Class)
		{
			Class = LoadObject<UClass>(nullptr, *PathString);
		}

		if (IsUsableActionClass(Class))
		{
			OutClasses.AddUnique(Class);
		}
	}
}

void EnsureActionClassMenuRefreshHooks()
{
	static bool bHooksInstalled = false;
	if (bHooksInstalled)
	{
		return;
	}
	bHooksInstalled = true;

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	auto Refresh = []()
	{
		FBlueprintActionDatabase::Get().RefreshClassActions(UK2Node_DIVEActionEvent::StaticClass());
	};

	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.OnFilesLoaded().AddLambda(Refresh);
	}

	AssetRegistry.OnAssetAdded().AddLambda(
		[Refresh](const FAssetData& AssetData)
		{
			if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
			{
				Refresh();
			}
		});
	AssetRegistry.OnAssetRemoved().AddLambda(
		[Refresh](const FAssetData& AssetData)
		{
			if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
			{
				Refresh();
			}
		});
	AssetRegistry.OnAssetUpdated().AddLambda(
		[Refresh](const FAssetData& AssetData)
		{
			if (AssetData.AssetClassPath == UBlueprint::StaticClass()->GetClassPathName())
			{
				Refresh();
			}
		});
}
} // namespace

UDIVEActionEventNodeSpawner* UDIVEActionEventNodeSpawner::Create(
	TSubclassOf<UEdGraphNode> NodeClass,
	TSubclassOf<UDIVEDeviceAction> InActionClass)
{
	check(NodeClass);
	check(InActionClass);

	UDIVEActionEventNodeSpawner* NodeSpawner = NewObject<UDIVEActionEventNodeSpawner>(GetTransientPackage());
	NodeSpawner->NodeClass = NodeClass;
	NodeSpawner->ActionClass = InActionClass;
	return NodeSpawner;
}

UEdGraphNode* UDIVEActionEventNodeSpawner::Invoke(
	UEdGraph* ParentGraph,
	FBindingSet const& Bindings,
	FVector2D const Location) const
{
	check(ParentGraph);
	UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(ParentGraph);

	if (!FBlueprintNodeTemplateCache::IsTemplateOuter(ParentGraph))
	{
		if (UK2Node* PreExistingNode = FindExistingNode(Blueprint))
		{
			return PreExistingNode;
		}
	}

	return Super::Invoke(ParentGraph, Bindings, Location);
}

UK2Node* UDIVEActionEventNodeSpawner::FindExistingNode(const UBlueprint* Blueprint) const
{
	if (!ActionClass)
	{
		return nullptr;
	}

	TArray<UK2Node_DIVEActionEvent*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_DIVEActionEvent>(Blueprint, Nodes);
	for (UK2Node_DIVEActionEvent* Node : Nodes)
	{
		if (Node && Node->ActionClass == ActionClass)
		{
			return Node;
		}
	}

	return nullptr;
}

UK2Node_DIVEActionEvent::UK2Node_DIVEActionEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UK2Node_DIVEActionEvent::GetActionName() const
{
	return ActionClass ? ActionClass->GetFName() : FName(TEXT("None"));
}

void UK2Node_DIVEActionEvent::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UClass* ActionPinClass = ActionClass
		? ActionClass.Get()
		: UDIVEDeviceAction::StaticClass();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, ActionPinClass, ActionPinName);

	UScriptStruct* ContextStruct = FDIVEActionContext::StaticStruct();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, ContextStruct, ContextPinName);

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_DIVEActionEvent::GetNodeTitleColor() const
{
	return FLinearColor(0.2f, 0.65f, 0.9f);
}

FText UK2Node_DIVEActionEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(
			TEXT("ActionName"),
			ActionClass
				? ActionClass->GetDisplayNameText()
				: LOCTEXT("NoneAction", "None"));
		CachedNodeTitle.SetCachedText(
			FText::Format(LOCTEXT("NodeTitle", "DIVE Action Event ({ActionName})"), Args),
			this);
	}
	return CachedNodeTitle;
}

FText UK2Node_DIVEActionEvent::GetTooltipText() const
{
	if (CachedTooltip.IsOutOfDate(this))
	{
		CachedTooltip.SetCachedText(
			LOCTEXT(
				"Tooltip",
				"Fires when the selected DIVE Device Action succeeds on this actor's Inspectable "
				"(instant Execute returned true, or continuous Begin succeeded). "
				"Requires UDIVEInspectableComponent on the actor."),
			this);
	}
	return CachedTooltip;
}

FSlateIcon UK2Node_DIVEActionEvent::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Event_16x");
	OutColor = GetNodeTitleColor();
	return Icon;
}

bool UK2Node_DIVEActionEvent::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
	if (!Super::IsCompatibleWithGraph(Graph))
	{
		return false;
	}

	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	return Blueprint && Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(AActor::StaticClass());
}

void UK2Node_DIVEActionEvent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_DIVEActionEvent, ActionClass))
	{
		CachedNodeTitle.MarkDirty();
		ReconstructNode();
	}
}

bool UK2Node_DIVEActionEvent::HasExternalDependencies(TArray<UStruct*>* OptionalOutput) const
{
	UClass* SourceClass = ActionClass.Get();
	const UBlueprint* SourceBlueprint = GetBlueprint();
	const bool bResult = SourceClass != nullptr
		&& (SourceBlueprint == nullptr || SourceClass->ClassGeneratedBy != SourceBlueprint);
	if (bResult && OptionalOutput)
	{
		OptionalOutput->AddUnique(SourceClass);
	}

	const bool bSuperResult = Super::HasExternalDependencies(OptionalOutput);
	return bResult || bSuperResult;
}

void UK2Node_DIVEActionEvent::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	if (!IsUsableActionClass(ActionClass))
	{
		MessageLog.Error(
			*LOCTEXT("InvalidActionClass", "@@ does not have a valid DIVE Device Action class.").ToString(),
			this);
		return;
	}

	const UBlueprint* Blueprint = GetBlueprint();
	if (Blueprint && Blueprint->ParentClass && !Blueprint->ParentClass->IsChildOf(AActor::StaticClass()))
	{
		MessageLog.Error(
			*LOCTEXT("NotActor", "@@ requires an Actor Blueprint (DIVE Inspectable lives on actors).").ToString(),
			this);
	}

	if (Blueprint && !BlueprintHasInspectableComponent(Blueprint))
	{
		MessageLog.Warning(
			*LOCTEXT(
				"NoInspectable",
				"@@: no UDIVEInspectableComponent found on this Blueprint. "
				"The event will not bind until an Inspectable exists on the instance.")
				.ToString(),
			this);
	}
}

void UK2Node_DIVEActionEvent::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(Schema);

	if (!IsUsableActionClass(ActionClass))
	{
		BreakAllNodeLinks();
		return;
	}

	UK2Node_DIVEActionBoundEvent* BoundEvent =
		CompilerContext.SpawnIntermediateNode<UK2Node_DIVEActionBoundEvent>(this, SourceGraph);
	BoundEvent->ActionClass = ActionClass;
	BoundEvent->CustomFunctionName = FName(*FString::Printf(
		TEXT("DIVEActEvt_%s_%s"),
		*GetActionName().ToString(),
		*BoundEvent->GetName()));
	BoundEvent->bInternalEvent = true;
	BoundEvent->bOverrideFunction = false;

	if (const FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
			UDIVEInspectableComponent::StaticClass(),
			GET_MEMBER_NAME_CHECKED(UDIVEInspectableComponent, OnActionExecuted)))
	{
		BoundEvent->EventReference.SetFromField<UFunction>(DelegateProp->SignatureFunction, false);
	}
	else
	{
		CompilerContext.MessageLog.Error(
			*LOCTEXT("MissingDelegate", "@@ could not resolve OnActionExecuted signature.").ToString(),
			this);
		BreakAllNodeLinks();
		return;
	}

	BoundEvent->AllocateDefaultPins();

	UK2Node_DynamicCast* CastNode =
		CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
	CastNode->TargetType = ActionClass;
	CastNode->SetPurity(false);
	CastNode->AllocateDefaultPins();

	UEdGraphPin* EventThen = Schema->FindExecutionPin(*BoundEvent, EGPD_Output);
	UEdGraphPin* EventAction = BoundEvent->FindPin(ActionPinName, EGPD_Output);
	UEdGraphPin* EventContext = BoundEvent->FindPin(ContextPinName, EGPD_Output);

	UEdGraphPin* CastExec = CastNode->GetExecPin();
	UEdGraphPin* CastSource = CastNode->GetCastSourcePin();
	UEdGraphPin* CastValid = CastNode->GetValidCastPin();
	UEdGraphPin* CastResult = CastNode->GetCastResultPin();

	if (!EventThen || !EventAction || !EventContext || !CastExec || !CastSource || !CastValid || !CastResult)
	{
		CompilerContext.MessageLog.Error(
			*LOCTEXT("ExpandPinFail", "@@ failed to expand DIVE Action Event pins.").ToString(),
			this);
		BreakAllNodeLinks();
		return;
	}

	Schema->TryCreateConnection(EventThen, CastExec);
	Schema->TryCreateConnection(EventAction, CastSource);

	UEdGraphPin* UserThen = FindPinChecked(UEdGraphSchema_K2::PN_Then);
	UEdGraphPin* UserAction = FindPinChecked(ActionPinName);
	UEdGraphPin* UserContext = FindPinChecked(ContextPinName);

	CompilerContext.MovePinLinksToIntermediate(*UserThen, *CastValid);
	CompilerContext.MovePinLinksToIntermediate(*UserAction, *CastResult);
	CompilerContext.MovePinLinksToIntermediate(*UserContext, *EventContext);

	BreakAllNodeLinks();
}

void UK2Node_DIVEActionEvent::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	auto CustomizeNode = [](UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TSubclassOf<UDIVEDeviceAction> InActionClass)
	{
		UK2Node_DIVEActionEvent* ActionNode = CastChecked<UK2Node_DIVEActionEvent>(NewNode);
		ActionNode->ActionClass = InActionClass;
	};

	if (ActionRegistrar.IsOpenForRegistration(GetClass()))
	{
		EnsureActionClassMenuRefreshHooks();

		TArray<UClass*> ActionClasses;
		CollectActionClasses(ActionClasses);

		for (UClass* Class : ActionClasses)
		{
			UDIVEActionEventNodeSpawner* NodeSpawner =
				UDIVEActionEventNodeSpawner::Create(GetClass(), Class);
			check(NodeSpawner);

			NodeSpawner->CustomizeNodeDelegate =
				UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
					CustomizeNode,
					TSubclassOf<UDIVEDeviceAction>(Class));
			ActionRegistrar.AddBlueprintAction(Class, NodeSpawner);
		}
	}
	else if (const UClass* ConstClass = Cast<UClass>(ActionRegistrar.GetActionKeyFilter()))
	{
		UClass* Class = const_cast<UClass*>(ConstClass);
		if (IsUsableActionClass(Class))
		{
			UDIVEActionEventNodeSpawner* NodeSpawner =
				UDIVEActionEventNodeSpawner::Create(GetClass(), Class);
			check(NodeSpawner);

			NodeSpawner->CustomizeNodeDelegate =
				UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(
					CustomizeNode,
					TSubclassOf<UDIVEDeviceAction>(Class));
			ActionRegistrar.AddBlueprintAction(Class, NodeSpawner);
		}
	}
}

FText UK2Node_DIVEActionEvent::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "DIVE|Events");
}

FBlueprintNodeSignature UK2Node_DIVEActionEvent::GetSignature() const
{
	FBlueprintNodeSignature NodeSignature = Super::GetSignature();
	NodeSignature.AddKeyValue(GetActionName().ToString());
	return NodeSignature;
}

TSharedPtr<FEdGraphSchemaAction> UK2Node_DIVEActionEvent::GetEventNodeAction(const FText& ActionCategory)
{
	TSharedPtr<FEdGraphSchemaAction_K2Event> EventNodeAction = MakeShareable(
		new FEdGraphSchemaAction_K2Event(
			ActionCategory,
			GetNodeTitle(ENodeTitleType::EditableTitle),
			GetTooltipText(),
			0));
	EventNodeAction->NodeTemplate = this;
	return EventNodeAction;
}

#undef LOCTEXT_NAMESPACE
