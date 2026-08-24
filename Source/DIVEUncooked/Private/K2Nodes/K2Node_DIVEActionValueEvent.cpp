// Copyright (c) 2026. All Rights Reserved.

#include "K2Nodes/K2Node_DIVEActionValueEvent.h"

#include "K2Nodes/DIVEActionEventNodeShared.h"
#include "K2Nodes/K2Node_DIVEActionValueBoundEvent.h"

#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSignature.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintNodeTemplateCache.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Engine/Blueprint.h"
#include "GameFramework/Actor.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_IfThenElse.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Styling/AppStyle.h"
#include "UObject/SoftObjectPath.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(K2Node_DIVEActionValueEvent)

#define LOCTEXT_NAMESPACE "K2Node_DIVEActionValueEvent"

namespace
{
const FName ActionPinName(TEXT("Action"));
const FName ContextPinName(TEXT("Context"));
const FName ValuePinName(TEXT("Value"));

bool IsUsableContinuousActionClass(const UClass* Class)
{
	return DIVEUncooked_IsUsableActionClass(Class, UDIVEContinuousDeviceAction::StaticClass());
}
} // namespace

UDIVEActionValueEventNodeSpawner* UDIVEActionValueEventNodeSpawner::Create(
	TSubclassOf<UEdGraphNode> NodeClass,
	const FSoftClassPath& InActionClassPath)
{
	check(NodeClass);
	check(InActionClassPath.IsValid());

	UDIVEActionValueEventNodeSpawner* NodeSpawner =
		NewObject<UDIVEActionValueEventNodeSpawner>(GetTransientPackage());
	NodeSpawner->NodeClass = NodeClass;
	NodeSpawner->ActionClassPath = InActionClassPath;
	if (UClass* LoadedClass = InActionClassPath.ResolveClass())
	{
		NodeSpawner->ActionClass = LoadedClass;
	}

	FString DisplayName = InActionClassPath.GetAssetName();
	if (DisplayName.EndsWith(TEXT("_C")))
	{
		DisplayName.LeftChopInline(2);
	}

	const FText ActionLabel = NodeSpawner->ActionClass
		? NodeSpawner->ActionClass->GetDisplayNameText()
		: FText::FromString(DisplayName);

	NodeSpawner->DefaultMenuSignature.MenuName = FText::Format(
		LOCTEXT("MenuNodeTitle", "DIVE Action Value Event ({0})"),
		ActionLabel);
	NodeSpawner->DefaultMenuSignature.Category = LOCTEXT("MenuCategory", "DIVE|Events");

	NodeSpawner->CustomizeNodeDelegate =
		UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda(
			[InActionClassPath](UEdGraphNode* NewNode, bool bIsTemplateNode)
			{
				UK2Node_DIVEActionValueEvent* ValueNode =
					CastChecked<UK2Node_DIVEActionValueEvent>(NewNode);
				UClass* Class = InActionClassPath.ResolveClass();
				if (!Class && !bIsTemplateNode)
				{
					Class = InActionClassPath.TryLoadClass<UDIVEContinuousDeviceAction>();
				}
				if (Class)
				{
					ValueNode->ActionClass = Class;
				}
			});

	return NodeSpawner;
}

UClass* UDIVEActionValueEventNodeSpawner::ResolveActionClass(const bool bLoadIfNeeded) const
{
	if (UClass* Loaded = ActionClass.Get())
	{
		return Loaded;
	}

	if (UClass* Existing = ActionClassPath.ResolveClass())
	{
		return Existing;
	}

	return bLoadIfNeeded ? ActionClassPath.TryLoadClass<UDIVEContinuousDeviceAction>() : nullptr;
}

UEdGraphNode* UDIVEActionValueEventNodeSpawner::Invoke(
	UEdGraph* ParentGraph,
	FBindingSet const& Bindings,
	FVector2D const Location) const
{
	check(ParentGraph);
	const bool bIsTemplate = FBlueprintNodeTemplateCache::IsTemplateOuter(ParentGraph);

	UClass* Resolved = ResolveActionClass(/*bLoadIfNeeded=*/ !bIsTemplate);
	if (Resolved)
	{
		const_cast<UDIVEActionValueEventNodeSpawner*>(this)->ActionClass = Resolved;
	}

	if (!bIsTemplate)
	{
		if (!IsUsableContinuousActionClass(Resolved))
		{
			return nullptr;
		}

		UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraphChecked(ParentGraph);
		if (UK2Node* PreExistingNode = FindExistingNode(Blueprint))
		{
			return PreExistingNode;
		}
	}

	return Super::Invoke(ParentGraph, Bindings, Location);
}

FBlueprintNodeSignature UDIVEActionValueEventNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSignature SpawnerSignature;
	SpawnerSignature.SetNodeClass(NodeClass);
	SpawnerSignature.AddKeyValue(ActionClassPath.ToString());
	return SpawnerSignature;
}

UK2Node* UDIVEActionValueEventNodeSpawner::FindExistingNode(const UBlueprint* Blueprint) const
{
	UClass* Resolved = ResolveActionClass(/*bLoadIfNeeded=*/ false);
	if (!Resolved)
	{
		return nullptr;
	}

	TArray<UK2Node_DIVEActionValueEvent*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_DIVEActionValueEvent>(Blueprint, Nodes);
	for (UK2Node_DIVEActionValueEvent* Node : Nodes)
	{
		if (Node && Node->ActionClass == Resolved && Node->BindingId.IsNone())
		{
			return Node;
		}
	}

	return nullptr;
}

UK2Node_DIVEActionValueEvent::UK2Node_DIVEActionValueEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FName UK2Node_DIVEActionValueEvent::GetActionName() const
{
	return ActionClass ? ActionClass->GetFName() : FName(TEXT("None"));
}

void UK2Node_DIVEActionValueEvent::AllocateDefaultPins()
{
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);

	UClass* ActionPinClass = ActionClass
		? ActionClass.Get()
		: UDIVEContinuousDeviceAction::StaticClass();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, ActionPinClass, ActionPinName);

	UScriptStruct* ContextStruct = FDIVEActionContext::StaticStruct();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, ContextStruct, ContextPinName);

	UScriptStruct* ValueStruct = FDIVEInteractionValue::StaticStruct();
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Struct, ValueStruct, ValuePinName);

	Super::AllocateDefaultPins();
}

FLinearColor UK2Node_DIVEActionValueEvent::GetNodeTitleColor() const
{
	return FLinearColor(0.35f, 0.75f, 0.55f);
}

FText UK2Node_DIVEActionValueEvent::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(
			TEXT("ActionName"),
			ActionClass
				? ActionClass->GetDisplayNameText()
				: LOCTEXT("NoneAction", "None"));
		if (BindingId.IsNone())
		{
			CachedNodeTitle.SetCachedText(
				FText::Format(LOCTEXT("NodeTitle", "DIVE Action Value Event ({ActionName})"), Args),
				this);
		}
		else
		{
			Args.Add(TEXT("BindingId"), FText::FromName(BindingId));
			CachedNodeTitle.SetCachedText(
				FText::Format(
					LOCTEXT("NodeTitleFiltered", "DIVE Action Value Event ({ActionName} · {BindingId})"),
					Args),
				this);
		}
	}
	return CachedNodeTitle;
}

FText UK2Node_DIVEActionValueEvent::GetTooltipText() const
{
	return LOCTEXT(
		"Tooltip",
		"Fires while the selected continuous DIVE Device Action reports live values on this actor's "
		"Inspectable. Optional BindingId filters to one catalog/component slot. "
		"Requires UDIVEInspectableComponent on the actor.");
}

FSlateIcon UK2Node_DIVEActionValueEvent::GetIconAndTint(FLinearColor& OutColor) const
{
	static FSlateIcon Icon(FAppStyle::GetAppStyleSetName(), "GraphEditor.Event_16x");
	OutColor = GetNodeTitleColor();
	return Icon;
}

bool UK2Node_DIVEActionValueEvent::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
	if (!Super::IsCompatibleWithGraph(Graph))
	{
		return false;
	}

	const UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
	return Blueprint && Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(AActor::StaticClass());
}

void UK2Node_DIVEActionValueEvent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_DIVEActionValueEvent, ActionClass)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_DIVEActionValueEvent, BindingId))
	{
		CachedNodeTitle.MarkDirty();
		ReconstructNode();
	}
}

bool UK2Node_DIVEActionValueEvent::HasExternalDependencies(TArray<UStruct*>* OptionalOutput) const
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

void UK2Node_DIVEActionValueEvent::ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const
{
	Super::ValidateNodeDuringCompilation(MessageLog);

	if (!IsUsableContinuousActionClass(ActionClass))
	{
		MessageLog.Error(
			*LOCTEXT(
				"InvalidActionClass",
				"@@ does not have a valid continuous DIVE Device Action class.")
				.ToString(),
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

	if (Blueprint && !DIVEUncooked_BlueprintHasInspectableComponent(Blueprint))
	{
		MessageLog.Warning(
			*LOCTEXT(
				"NoInspectable",
				"@@: no UDIVEInspectableComponent found on this Blueprint. "
				"The event will not bind until an Inspectable exists on the instance.")
				.ToString(),
			this);
	}

	if (Blueprint)
	{
		TArray<UK2Node_DIVEActionValueEvent*> Nodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_DIVEActionValueEvent>(Blueprint, Nodes);
		for (const UK2Node_DIVEActionValueEvent* Other : Nodes)
		{
			if (Other && Other != this
				&& Other->GetGraph() == GetGraph()
				&& Other->ActionClass == ActionClass
				&& Other->BindingId == BindingId)
			{
				MessageLog.Warning(
					*LOCTEXT(
						"DuplicateFilter",
						"@@ duplicates another DIVE Action Value Event with the same Action class and BindingId.")
						.ToString(),
					this);
				break;
			}
		}
	}
}

void UK2Node_DIVEActionValueEvent::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();
	check(Schema);

	if (!IsUsableContinuousActionClass(ActionClass))
	{
		BreakAllNodeLinks();
		return;
	}

	UK2Node_DIVEActionValueBoundEvent* BoundEvent =
		CompilerContext.SpawnIntermediateNode<UK2Node_DIVEActionValueBoundEvent>(this, SourceGraph);
	BoundEvent->CustomFunctionName = FName(*FString::Printf(
		TEXT("DIVEActValEvt_%s_%s_%s"),
		*GetActionName().ToString(),
		BindingId.IsNone() ? TEXT("Any") : *BindingId.ToString(),
		*BoundEvent->GetName()));
	BoundEvent->bOverrideFunction = false;

	if (const FMulticastDelegateProperty* DelegateProp = FindFProperty<FMulticastDelegateProperty>(
			UDIVEInspectableComponent::StaticClass(),
			GET_MEMBER_NAME_CHECKED(UDIVEInspectableComponent, OnActionValueChanged)))
	{
		BoundEvent->EventReference.SetFromField<UFunction>(DelegateProp->SignatureFunction, false);
	}
	else
	{
		CompilerContext.MessageLog.Error(
			*LOCTEXT("MissingDelegate", "@@ could not resolve OnActionValueChanged signature.").ToString(),
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
	UEdGraphPin* EventValue = BoundEvent->FindPin(ValuePinName, EGPD_Output);

	UEdGraphPin* CastExec = CastNode->GetExecPin();
	UEdGraphPin* CastSource = CastNode->GetCastSourcePin();
	UEdGraphPin* CastValid = CastNode->GetValidCastPin();
	UEdGraphPin* CastResult = CastNode->GetCastResultPin();

	if (!EventThen || !EventAction || !EventContext || !EventValue
		|| !CastExec || !CastSource || !CastValid || !CastResult)
	{
		CompilerContext.MessageLog.Error(
			*LOCTEXT("ExpandPinFail", "@@ failed to expand DIVE Action Value Event pins.").ToString(),
			this);
		BreakAllNodeLinks();
		return;
	}

	Schema->TryCreateConnection(EventThen, CastExec);
	Schema->TryCreateConnection(EventAction, CastSource);

	UEdGraphPin* UserThen = FindPinChecked(UEdGraphSchema_K2::PN_Then);
	UEdGraphPin* UserAction = FindPinChecked(ActionPinName);
	UEdGraphPin* UserContext = FindPinChecked(ContextPinName);
	UEdGraphPin* UserValue = FindPinChecked(ValuePinName);

	UEdGraphPin* FilteredThen = CastValid;
	if (!BindingId.IsNone())
	{
		UK2Node_BreakStruct* BreakContext =
			CompilerContext.SpawnIntermediateNode<UK2Node_BreakStruct>(this, SourceGraph);
		BreakContext->StructType = FDIVEActionContext::StaticStruct();
		BreakContext->bMadeAfterOverridePinRemoval = true;
		BreakContext->AllocateDefaultPins();

		UEdGraphPin* BreakInput = BreakContext->FindPin(
			FDIVEActionContext::StaticStruct()->GetFName(),
			EGPD_Input);
		UEdGraphPin* BreakBindingId = BreakContext->FindPin(
			GET_MEMBER_NAME_CHECKED(FDIVEActionContext, BindingId),
			EGPD_Output);
		if (!BreakInput || !BreakBindingId)
		{
			CompilerContext.MessageLog.Error(
				*LOCTEXT("BreakContextFail", "@@ failed to break FDIVEActionContext for BindingId filter.").ToString(),
				this);
			BreakAllNodeLinks();
			return;
		}

		Schema->TryCreateConnection(EventContext, BreakInput);

		UK2Node_CallFunction* EqualsNode =
			CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		const UFunction* EqualFunction = UKismetMathLibrary::StaticClass()->FindFunctionByName(
			GET_FUNCTION_NAME_CHECKED(UKismetMathLibrary, EqualEqual_NameName));
		if (!EqualFunction)
		{
			CompilerContext.MessageLog.Error(
				*LOCTEXT("EqualsFnFail", "@@ could not resolve Name equality for BindingId filter.").ToString(),
				this);
			BreakAllNodeLinks();
			return;
		}
		EqualsNode->SetFromFunction(EqualFunction);
		EqualsNode->AllocateDefaultPins();

		UEdGraphPin* EqualsA = EqualsNode->FindPinChecked(TEXT("A"));
		UEdGraphPin* EqualsB = EqualsNode->FindPinChecked(TEXT("B"));
		UEdGraphPin* EqualsResult = EqualsNode->GetReturnValuePin();
		if (!EqualsResult)
		{
			CompilerContext.MessageLog.Error(
				*LOCTEXT("EqualsResultFail", "@@ Name equality node has no return pin for BindingId filter.").ToString(),
				this);
			BreakAllNodeLinks();
			return;
		}

		Schema->TryCreateConnection(BreakBindingId, EqualsA);
		Schema->TrySetDefaultValue(*EqualsB, BindingId.ToString());

		UK2Node_IfThenElse* Branch =
			CompilerContext.SpawnIntermediateNode<UK2Node_IfThenElse>(this, SourceGraph);
		Branch->AllocateDefaultPins();

		UEdGraphPin* BranchExec = Branch->GetExecPin();
		UEdGraphPin* BranchCond = Branch->GetConditionPin();
		UEdGraphPin* BranchThen = Branch->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
		if (!BranchExec || !BranchCond || !BranchThen)
		{
			CompilerContext.MessageLog.Error(
				*LOCTEXT("BranchFail", "@@ failed to expand BindingId branch.").ToString(),
				this);
			BreakAllNodeLinks();
			return;
		}

		Schema->TryCreateConnection(CastValid, BranchExec);
		Schema->TryCreateConnection(EqualsResult, BranchCond);
		FilteredThen = BranchThen;
	}

	CompilerContext.MovePinLinksToIntermediate(*UserThen, *FilteredThen);
	CompilerContext.MovePinLinksToIntermediate(*UserAction, *CastResult);
	CompilerContext.MovePinLinksToIntermediate(*UserContext, *EventContext);
	CompilerContext.MovePinLinksToIntermediate(*UserValue, *EventValue);

	BreakAllNodeLinks();
}

void UK2Node_DIVEActionValueEvent::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	DIVEUncooked_RegisterActionClassMenuActions(
		GetClass(),
		ActionRegistrar,
		UDIVEContinuousDeviceAction::StaticClass(),
		[this](const FSoftClassPath& ClassPath) -> UBlueprintNodeSpawner*
		{
			return UDIVEActionValueEventNodeSpawner::Create(GetClass(), ClassPath);
		});
}

FText UK2Node_DIVEActionValueEvent::GetMenuCategory() const
{
	return LOCTEXT("MenuCategory", "DIVE|Events");
}

FBlueprintNodeSignature UK2Node_DIVEActionValueEvent::GetSignature() const
{
	FBlueprintNodeSignature NodeSignature = Super::GetSignature();
	NodeSignature.AddKeyValue(GetActionName().ToString());
	if (!BindingId.IsNone())
	{
		NodeSignature.AddKeyValue(BindingId.ToString());
	}
	return NodeSignature;
}

TArray<FName> UK2Node_DIVEActionValueEvent::GetAvailableBindingIds() const
{
	return DIVEUncooked_GetAvailableBindingIds(GetBlueprint());
}

TSharedPtr<FEdGraphSchemaAction> UK2Node_DIVEActionValueEvent::GetEventNodeAction(const FText& ActionCategory)
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
