// Copyright (c) 2026. All Rights Reserved.

#include "K2Nodes/K2Node_DIVEActionEvent.h"

#include "K2Nodes/K2Node_DIVEActionBoundEvent.h"

#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/BlueprintSupport.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSignature.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintNodeTemplateCache.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphSchema_K2_Actions.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_IfThenElse.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "UObject/Class.h"
#include "UObject/SoftObjectPath.h"

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

bool IsNativeScriptClassPath(const FTopLevelAssetPath& ClassPath)
{
	return ClassPath.GetPackageName().ToString().StartsWith(TEXT("/Script/"));
}

FSoftObjectPath BlueprintAssetPathFromGeneratedClass(const FTopLevelAssetPath& GeneratedClassPath)
{
	FString PathString = GeneratedClassPath.ToString();
	if (PathString.EndsWith(TEXT("_C")))
	{
		PathString.LeftChopInline(2);
	}
	return FSoftObjectPath(PathString);
}

FAssetData FindBlueprintAssetData(IAssetRegistry& AssetRegistry, const FTopLevelAssetPath& GeneratedClassPath)
{
	const FSoftObjectPath BlueprintPath = BlueprintAssetPathFromGeneratedClass(GeneratedClassPath);
	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(BlueprintPath);
	if (AssetData.IsValid())
	{
		return AssetData;
	}

	return AssetRegistry.GetAssetByObjectPath(FSoftObjectPath(GeneratedClassPath.ToString()));
}

bool IsDeviceActionBlueprintAsset(const FAssetData& AssetData)
{
	if (!AssetData.IsValid()
		|| AssetData.AssetClassPath != UBlueprint::StaticClass()->GetClassPathName())
	{
		return false;
	}

	FString NativeParentExport;
	if (!AssetData.GetTagValue(FBlueprintTags::NativeParentClassPath, NativeParentExport)
		|| NativeParentExport.IsEmpty())
	{
		return false;
	}

	const FString NativeParentPath = FPackageName::ExportTextPathToObjectPath(NativeParentExport);
	const UClass* NativeParent = UClass::TryFindTypeSlow<UClass>(NativeParentPath);
	return NativeParent && NativeParent->IsChildOf(UDIVEDeviceAction::StaticClass());
}

void CollectActionClassEntries(TArray<UClass*>& OutLoaded, TArray<FSoftClassPath>& OutUnloaded)
{
	OutLoaded.Reset();
	OutUnloaded.Reset();

	TSet<FTopLevelAssetPath> Seen;

	TArray<UClass*> Derived;
	GetDerivedClasses(UDIVEDeviceAction::StaticClass(), Derived, /*bRecursive=*/true);
	for (UClass* Class : Derived)
	{
		if (IsUsableActionClass(Class))
		{
			OutLoaded.Add(Class);
			Seen.Add(Class->GetClassPathName());
		}
	}

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TArray<FTopLevelAssetPath> BaseClasses;
	BaseClasses.Add(UDIVEDeviceAction::StaticClass()->GetClassPathName());

	TSet<FTopLevelAssetPath> Excluded;
	TSet<FTopLevelAssetPath> DerivedPaths;
	AssetRegistry.GetDerivedClassNames(BaseClasses, Excluded, DerivedPaths);

	for (const FTopLevelAssetPath& ClassPath : DerivedPaths)
	{
		if (Seen.Contains(ClassPath))
		{
			continue;
		}

		const FString PathString = ClassPath.ToString();
		if (UClass* Class = FindObject<UClass>(nullptr, *PathString))
		{
			if (IsUsableActionClass(Class))
			{
				OutLoaded.Add(Class);
				Seen.Add(ClassPath);
			}
			continue;
		}

		if (IsNativeScriptClassPath(ClassPath))
		{
			continue;
		}

		const FString AssetName = ClassPath.GetAssetName().ToString();
		if (AssetName == TEXT("DIVEDeviceAction")
			|| AssetName == TEXT("DIVEContinuousDeviceAction")
			|| AssetName.StartsWith(TEXT("SKEL_"))
			|| AssetName.StartsWith(TEXT("REINST_"))
			|| AssetName.StartsWith(TEXT("TRASHCLASS_")))
		{
			continue;
		}

		OutUnloaded.Add(FSoftClassPath(PathString));
		Seen.Add(ClassPath);
	}
}

FDelegateHandle GActionClassMenuOnFilesLoadedHandle;
FDelegateHandle GActionClassMenuOnAssetAddedHandle;
FDelegateHandle GActionClassMenuOnAssetRemovedHandle;
FDelegateHandle GActionClassMenuOnAssetUpdatedHandle;
bool bActionClassMenuHooksInstalled = false;

const UDIVEInspectableComponent* FindInspectableTemplate(const UBlueprint* Blueprint)
{
	for (const UBlueprint* Current = Blueprint; Current; )
	{
		if (Current->SimpleConstructionScript)
		{
			for (const USCS_Node* Node : Current->SimpleConstructionScript->GetAllNodes())
			{
				if (const UDIVEInspectableComponent* Comp =
					Cast<UDIVEInspectableComponent>(Node->ComponentTemplate))
				{
					return Comp;
				}
			}
		}

		if (const UClass* Generated = Current->GeneratedClass
			? Current->GeneratedClass
			: Current->SkeletonGeneratedClass)
		{
			if (const AActor* CDO = Cast<AActor>(Generated->GetDefaultObject()))
			{
				if (const UDIVEInspectableComponent* Comp =
					CDO->FindComponentByClass<UDIVEInspectableComponent>())
				{
					return Comp;
				}
			}
		}

		const UClass* ParentClass = Current->ParentClass;
		Current = ParentClass ? Cast<UBlueprint>(ParentClass->ClassGeneratedBy) : nullptr;
	}

	return nullptr;
}
} // namespace

void DIVEUncooked_EnsureActionClassMenuRefreshHooks()
{
	if (bActionClassMenuHooksInstalled)
	{
		return;
	}
	bActionClassMenuHooksInstalled = true;

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	auto Refresh = []()
	{
		FBlueprintActionDatabase::Get().RefreshClassActions(UK2Node_DIVEActionEvent::StaticClass());
	};

	if (AssetRegistry.IsLoadingAssets())
	{
		GActionClassMenuOnFilesLoadedHandle = AssetRegistry.OnFilesLoaded().AddLambda(Refresh);
	}

	auto MaybeRefresh = [Refresh](const FAssetData& AssetData)
	{
		if (IsDeviceActionBlueprintAsset(AssetData))
		{
			Refresh();
		}
	};

	GActionClassMenuOnAssetAddedHandle = AssetRegistry.OnAssetAdded().AddLambda(MaybeRefresh);
	GActionClassMenuOnAssetRemovedHandle = AssetRegistry.OnAssetRemoved().AddLambda(MaybeRefresh);
	GActionClassMenuOnAssetUpdatedHandle = AssetRegistry.OnAssetUpdated().AddLambda(MaybeRefresh);
}

void DIVEUncooked_UninstallActionClassMenuRefreshHooks()
{
	if (!bActionClassMenuHooksInstalled)
	{
		return;
	}

	if (FModuleManager::Get().IsModuleLoaded(TEXT("AssetRegistry")))
	{
		IAssetRegistry& AssetRegistry =
			FModuleManager::GetModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		AssetRegistry.OnFilesLoaded().Remove(GActionClassMenuOnFilesLoadedHandle);
		AssetRegistry.OnAssetAdded().Remove(GActionClassMenuOnAssetAddedHandle);
		AssetRegistry.OnAssetRemoved().Remove(GActionClassMenuOnAssetRemovedHandle);
		AssetRegistry.OnAssetUpdated().Remove(GActionClassMenuOnAssetUpdatedHandle);
	}

	GActionClassMenuOnFilesLoadedHandle.Reset();
	GActionClassMenuOnAssetAddedHandle.Reset();
	GActionClassMenuOnAssetRemovedHandle.Reset();
	GActionClassMenuOnAssetUpdatedHandle.Reset();
	bActionClassMenuHooksInstalled = false;
}

UDIVEActionEventNodeSpawner* UDIVEActionEventNodeSpawner::Create(
	TSubclassOf<UEdGraphNode> NodeClass,
	const FSoftClassPath& InActionClassPath)
{
	check(NodeClass);
	check(InActionClassPath.IsValid());

	UDIVEActionEventNodeSpawner* NodeSpawner = NewObject<UDIVEActionEventNodeSpawner>(GetTransientPackage());
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
		LOCTEXT("MenuNodeTitle", "DIVE Action Event ({0})"),
		ActionLabel);
	NodeSpawner->DefaultMenuSignature.Category = LOCTEXT("MenuCategory", "DIVE|Events");

	NodeSpawner->CustomizeNodeDelegate =
		UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateLambda(
			[InActionClassPath](UEdGraphNode* NewNode, bool bIsTemplateNode)
			{
				UK2Node_DIVEActionEvent* ActionNode = CastChecked<UK2Node_DIVEActionEvent>(NewNode);
				UClass* Class = InActionClassPath.ResolveClass();
				if (!Class && !bIsTemplateNode)
				{
					Class = InActionClassPath.TryLoadClass<UDIVEDeviceAction>();
				}
				if (Class)
				{
					ActionNode->ActionClass = Class;
				}
			});

	return NodeSpawner;
}

UClass* UDIVEActionEventNodeSpawner::ResolveActionClass(const bool bLoadIfNeeded) const
{
	if (UClass* Loaded = ActionClass.Get())
	{
		return Loaded;
	}

	if (UClass* Existing = ActionClassPath.ResolveClass())
	{
		return Existing;
	}

	return bLoadIfNeeded ? ActionClassPath.TryLoadClass<UDIVEDeviceAction>() : nullptr;
}

UEdGraphNode* UDIVEActionEventNodeSpawner::Invoke(
	UEdGraph* ParentGraph,
	FBindingSet const& Bindings,
	FVector2D const Location) const
{
	check(ParentGraph);
	const bool bIsTemplate = FBlueprintNodeTemplateCache::IsTemplateOuter(ParentGraph);

	UClass* Resolved = ResolveActionClass(/*bLoadIfNeeded=*/ !bIsTemplate);
	if (Resolved)
	{
		const_cast<UDIVEActionEventNodeSpawner*>(this)->ActionClass = Resolved;
	}

	if (!bIsTemplate)
	{
		if (!IsUsableActionClass(Resolved))
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

FBlueprintNodeSignature UDIVEActionEventNodeSpawner::GetSpawnerSignature() const
{
	FBlueprintNodeSignature SpawnerSignature;
	SpawnerSignature.SetNodeClass(NodeClass);
	SpawnerSignature.AddKeyValue(ActionClassPath.ToString());
	return SpawnerSignature;
}

UK2Node* UDIVEActionEventNodeSpawner::FindExistingNode(const UBlueprint* Blueprint) const
{
	UClass* Resolved = ResolveActionClass(/*bLoadIfNeeded=*/ false);
	if (!Resolved)
	{
		return nullptr;
	}

	TArray<UK2Node_DIVEActionEvent*> Nodes;
	FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_DIVEActionEvent>(Blueprint, Nodes);
	for (UK2Node_DIVEActionEvent* Node : Nodes)
	{
		if (Node && Node->ActionClass == Resolved && Node->BindingId.IsNone())
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
		if (BindingId.IsNone())
		{
			CachedNodeTitle.SetCachedText(
				FText::Format(LOCTEXT("NodeTitle", "DIVE Action Event ({ActionName})"), Args),
				this);
		}
		else
		{
			Args.Add(TEXT("BindingId"), FText::FromName(BindingId));
			CachedNodeTitle.SetCachedText(
				FText::Format(LOCTEXT("NodeTitleFiltered", "DIVE Action Event ({ActionName} · {BindingId})"), Args),
				this);
		}
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
				"Optional BindingId filters to one catalog/component slot. "
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
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_DIVEActionEvent, ActionClass)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(UK2Node_DIVEActionEvent, BindingId))
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

	if (Blueprint)
	{
		TArray<UK2Node_DIVEActionEvent*> Nodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UK2Node_DIVEActionEvent>(Blueprint, Nodes);
		for (const UK2Node_DIVEActionEvent* Other : Nodes)
		{
			if (Other && Other != this
				&& Other->GetGraph() == GetGraph()
				&& Other->ActionClass == ActionClass
				&& Other->BindingId == BindingId)
			{
				MessageLog.Warning(
					*LOCTEXT(
						"DuplicateFilter",
						"@@ duplicates another DIVE Action Event with the same Action class and BindingId.")
						.ToString(),
					this);
				break;
			}
		}
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
	BoundEvent->CustomFunctionName = FName(*FString::Printf(
		TEXT("DIVEActEvt_%s_%s_%s"),
		*GetActionName().ToString(),
		BindingId.IsNone() ? TEXT("Any") : *BindingId.ToString(),
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

	BreakAllNodeLinks();
}

void UK2Node_DIVEActionEvent::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	auto RegisterLoadedClass = [this, &ActionRegistrar](UClass* Class)
	{
		if (!IsUsableActionClass(Class) || !ActionRegistrar.IsOpenForRegistration(Class))
		{
			return;
		}

		UDIVEActionEventNodeSpawner* NodeSpawner =
			UDIVEActionEventNodeSpawner::Create(GetClass(), FSoftClassPath(Class));
		if (NodeSpawner)
		{
			// Native: key = UClass. Blueprint generated: ResolveClassKey maps BPGC → Blueprint
			// asset (same key as FAssetData of the loaded Blueprint). Passing UClass keeps the
			// filter branch working when ActionKeyFilter is the generated class.
			ActionRegistrar.AddBlueprintAction(Class, NodeSpawner);
		}
	};

	auto RegisterUnloadedClass = [this, &ActionRegistrar](const FSoftClassPath& ClassPath)
	{
		if (!ClassPath.IsValid())
		{
			return;
		}

		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		const FAssetData AssetData = FindBlueprintAssetData(
			AssetRegistry,
			ClassPath.GetAssetPath());
		if (!AssetData.IsValid() || !ActionRegistrar.IsOpenForRegistration(AssetData))
		{
			return;
		}

		UDIVEActionEventNodeSpawner* NodeSpawner =
			UDIVEActionEventNodeSpawner::Create(GetClass(), ClassPath);
		if (NodeSpawner)
		{
			ActionRegistrar.AddBlueprintAction(AssetData, NodeSpawner);
		}
	};

	if (ActionRegistrar.IsOpenForRegistration(GetClass()))
	{
		DIVEUncooked_EnsureActionClassMenuRefreshHooks();

		TArray<UClass*> LoadedClasses;
		TArray<FSoftClassPath> UnloadedClasses;
		CollectActionClassEntries(LoadedClasses, UnloadedClasses);

		for (UClass* Class : LoadedClasses)
		{
			RegisterLoadedClass(Class);
		}
		for (const FSoftClassPath& ClassPath : UnloadedClasses)
		{
			RegisterUnloadedClass(ClassPath);
		}
	}
	else if (const UClass* ConstClass = Cast<UClass>(ActionRegistrar.GetActionKeyFilter()))
	{
		RegisterLoadedClass(const_cast<UClass*>(ConstClass));
	}
	else if (const UBlueprint* Blueprint = Cast<UBlueprint>(ActionRegistrar.GetActionKeyFilter()))
	{
		UClass* Generated = Blueprint->GeneratedClass
			? Blueprint->GeneratedClass
			: Blueprint->SkeletonGeneratedClass;
		if (IsUsableActionClass(Generated))
		{
			RegisterLoadedClass(Generated);
		}
		else if (!Generated)
		{
			RegisterUnloadedClass(FSoftClassPath(Blueprint->GetPathName() + TEXT("_C")));
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
	if (!BindingId.IsNone())
	{
		NodeSignature.AddKeyValue(BindingId.ToString());
	}
	return NodeSignature;
}

TArray<FName> UK2Node_DIVEActionEvent::GetAvailableBindingIds() const
{
	TArray<FName> Result;
	Result.Add(NAME_None);
	if (const UDIVEInspectableComponent* Inspectable = FindInspectableTemplate(GetBlueprint()))
	{
		for (const FName Id : Inspectable->GetAvailableBindingIds())
		{
			Result.AddUnique(Id);
		}
	}
	return Result;
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
