// Copyright (c) 2026. All Rights Reserved.

#include "K2Nodes/DIVEActionEventNodeShared.h"

#include "K2Nodes/K2Node_DIVEActionEvent.h"
#include "K2Nodes/K2Node_DIVEActionValueEvent.h"

#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Blueprint/BlueprintSupport.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"

namespace
{
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

void CollectActionClassEntries(
	const UClass* BaseClass,
	TArray<UClass*>& OutLoaded,
	TArray<FSoftClassPath>& OutUnloaded)
{
	OutLoaded.Reset();
	OutUnloaded.Reset();
	if (!BaseClass)
	{
		return;
	}

	TSet<FTopLevelAssetPath> Seen;

	TArray<UClass*> Derived;
	GetDerivedClasses(BaseClass, Derived, /*bRecursive=*/true);
	for (UClass* Class : Derived)
	{
		if (DIVEUncooked_IsUsableActionClass(Class, BaseClass))
		{
			OutLoaded.Add(Class);
			Seen.Add(Class->GetClassPathName());
		}
	}

	IAssetRegistry& AssetRegistry =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();

	TArray<FTopLevelAssetPath> BaseClasses;
	BaseClasses.Add(BaseClass->GetClassPathName());

	TSet<FTopLevelAssetPath> Excluded;
	TSet<FTopLevelAssetPath> DerivedPaths;
	AssetRegistry.GetDerivedClassNames(BaseClasses, Excluded, DerivedPaths);

	const FString BaseName = BaseClass->GetName();
	for (const FTopLevelAssetPath& ClassPath : DerivedPaths)
	{
		if (Seen.Contains(ClassPath))
		{
			continue;
		}

		const FString PathString = ClassPath.ToString();
		if (UClass* Class = FindObject<UClass>(nullptr, *PathString))
		{
			if (DIVEUncooked_IsUsableActionClass(Class, BaseClass))
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
		if (AssetName == BaseName
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

FDelegateHandle GActionClassMenuOnFilesLoadedHandle;
FDelegateHandle GActionClassMenuOnAssetAddedHandle;
FDelegateHandle GActionClassMenuOnAssetRemovedHandle;
FDelegateHandle GActionClassMenuOnAssetUpdatedHandle;
bool bActionClassMenuHooksInstalled = false;
} // namespace

bool DIVEUncooked_IsUsableActionClass(const UClass* Class, const UClass* BaseClass)
{
	if (!Class || !BaseClass || !Class->IsChildOf(BaseClass) || Class == BaseClass)
	{
		return false;
	}

	if (Class->HasAnyClassFlags(
			CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_Hidden | CLASS_HideDropDown))
	{
		return false;
	}

	const FString Name = Class->GetName();
	return !Name.StartsWith(TEXT("SKEL_"))
		&& !Name.StartsWith(TEXT("REINST_"))
		&& !Name.StartsWith(TEXT("TRASHCLASS_"));
}

bool DIVEUncooked_BlueprintHasInspectableComponent(const UBlueprint* Blueprint)
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
			if (ObjProp && ObjProp->PropertyClass
				&& ObjProp->PropertyClass->IsChildOf(UDIVEInspectableComponent::StaticClass()))
			{
				return true;
			}
		}
		return false;
	};

	if (HasInspectableInClass(Blueprint->SkeletonGeneratedClass)
		|| HasInspectableInClass(Blueprint->GeneratedClass))
	{
		return true;
	}

	if (Blueprint->SimpleConstructionScript)
	{
		for (const USCS_Node* Node : Blueprint->SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->ComponentClass
				&& Node->ComponentClass->IsChildOf(UDIVEInspectableComponent::StaticClass()))
			{
				return true;
			}
		}
	}

	return false;
}

TArray<FName> DIVEUncooked_GetAvailableBindingIds(const UBlueprint* Blueprint)
{
	TArray<FName> Result;
	Result.Add(NAME_None);
	if (const UDIVEInspectableComponent* Inspectable = FindInspectableTemplate(Blueprint))
	{
		for (const FName Id : Inspectable->GetAvailableBindingIds())
		{
			Result.AddUnique(Id);
		}
	}
	return Result;
}

void DIVEUncooked_RegisterActionClassMenuActions(
	const UClass* NodeClass,
	FBlueprintActionDatabaseRegistrar& ActionRegistrar,
	const UClass* ActionBaseClass,
	TFunctionRef<UBlueprintNodeSpawner*(const FSoftClassPath&)> CreateSpawner)
{
	auto RegisterLoadedClass = [&ActionRegistrar, ActionBaseClass, &CreateSpawner](UClass* Class)
	{
		if (!DIVEUncooked_IsUsableActionClass(Class, ActionBaseClass)
			|| !ActionRegistrar.IsOpenForRegistration(Class))
		{
			return;
		}

		ActionRegistrar.AddBlueprintAction(Class, CreateSpawner(FSoftClassPath(Class)));
	};

	auto RegisterUnloadedClass = [&ActionRegistrar, &CreateSpawner](const FSoftClassPath& ClassPath)
	{
		if (!ClassPath.IsValid())
		{
			return;
		}

		IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
		const FAssetData AssetData = FindBlueprintAssetData(AssetRegistry, ClassPath.GetAssetPath());
		if (!AssetData.IsValid() || !ActionRegistrar.IsOpenForRegistration(AssetData))
		{
			return;
		}

		ActionRegistrar.AddBlueprintAction(AssetData, CreateSpawner(ClassPath));
	};

	if (ActionRegistrar.IsOpenForRegistration(NodeClass))
	{
		DIVEUncooked_EnsureActionClassMenuRefreshHooks();

		TArray<UClass*> LoadedClasses;
		TArray<FSoftClassPath> UnloadedClasses;
		CollectActionClassEntries(ActionBaseClass, LoadedClasses, UnloadedClasses);

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
		if (DIVEUncooked_IsUsableActionClass(Generated, ActionBaseClass))
		{
			RegisterLoadedClass(Generated);
		}
		else if (!Generated)
		{
			RegisterUnloadedClass(FSoftClassPath(Blueprint->GetPathName() + TEXT("_C")));
		}
	}
}

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
		FBlueprintActionDatabase::Get().RefreshClassActions(UK2Node_DIVEActionValueEvent::StaticClass());
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
