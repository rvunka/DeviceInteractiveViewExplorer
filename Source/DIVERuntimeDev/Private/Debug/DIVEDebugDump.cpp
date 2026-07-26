// Copyright (c) 2026. All Rights Reserved.

#include "Debug/DIVEDebugDump.h"

#include "DIVEConvention.h"
#include "DIVEDeviceActionHandler.h"
#include "DIVEDeviceActionResolve.h"
#include "DIVEHierarchy.h"
#include "DIVEInspectableComponent.h"
#include "DIVELog.h"
#include "DIVESessionSubsystem.h"
#include "DIVETypes.h"

#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

namespace
{
FString GLastDumpFilePath;
TArray<IConsoleObject*> GRegisteredConsoleObjects;

void AppendLine(FString& Out, const FString& Line)
{
	Out += Line;
	Out += TEXT("\n");
}

FString YesNo(const bool bValue)
{
	return bValue ? TEXT("yes") : TEXT("no");
}

bool HasBoolProperty(const AActor* Owner, const FName PropertyName)
{
	if (!Owner || PropertyName.IsNone())
	{
		return false;
	}

	if (FindFProperty<FBoolProperty>(Owner->GetClass(), PropertyName))
	{
		return true;
	}

	const FString Wanted = PropertyName.ToString();
	for (TFieldIterator<FBoolProperty> It(Owner->GetClass()); It; ++It)
	{
		const FBoolProperty* BoolProperty = *It;
		if (BoolProperty
			&& (BoolProperty->GetFName() == PropertyName
				|| BoolProperty->GetName() == Wanted
				|| BoolProperty->GetAuthoredName() == Wanted))
		{
			return true;
		}
	}

	return false;
}

bool HasBoolFunction(const AActor* Owner, const FName FunctionName)
{
	if (!Owner || FunctionName.IsNone())
	{
		return false;
	}

	const UFunction* Function = Owner->FindFunction(FunctionName);
	return Function && CastField<FBoolProperty>(Function->GetReturnProperty()) != nullptr;
}

FString WriteDumpFile(const FString& Tag, const FString& Body)
{
	GLastDumpFilePath.Reset();

	const FString Dir = FPaths::ProjectSavedDir() / TEXT("DIVE") / TEXT("Dumps");
	IFileManager::Get().MakeDirectory(*Dir, true);

	const FString FileName = FString::Printf(
		TEXT("DIVEDump_%s_%s.txt"),
		*Tag,
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	const FString FullPath = Dir / FileName;

	if (FFileHelper::SaveStringToFile(Body, *FullPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		GLastDumpFilePath = FPaths::ConvertRelativePathToFull(FullPath);
		UE_LOG(LogDIVE, Log, TEXT("DIVE dump file: %s"), *GLastDumpFilePath);
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				6.0f,
				FColor::Cyan,
				FString::Printf(TEXT("DIVE dump saved: Saved/DIVE/Dumps/%s"), *FileName));
		}
		return GLastDumpFilePath;
	}

	UE_LOG(LogDIVE, Warning, TEXT("DIVE dump file write failed: %s"), *FullPath);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			6.0f,
			FColor::Red,
			TEXT("DIVE dump FAILED to write file — check Output Log (LogDIVE)."));
	}
	return FString();
}

void DumpCatalogEntry(
	FString& Out,
	AActor* Owner,
	UDIVEInspectableComponent* Inspectable,
	const FName CatalogKey,
	const FDIVEPickContextMenuActionList& ActionList)
{
	AppendLine(Out, FString::Printf(TEXT("  CatalogKey='%s' Actions=%d Primary='%s'"),
		*CatalogKey.ToString(),
		ActionList.Actions.Num(),
		*ActionList.PrimaryActionId.ToString()));

	if (ActionList.Actions.IsEmpty())
	{
		AppendLine(Out, TEXT("    WARNING: Actions array is EMPTY — no custom menu rows for this key."));
	}

	FDIVEFocusTarget Resolved;
	const bool bResolved = Inspectable->TryResolveStartFocusTarget(CatalogKey, Resolved);
	if (!bResolved)
	{
		AppendLine(Out, TEXT("    Resolve: FAIL (no anchor PartId / pickable primitive with this FName)"));
	}
	else if (Resolved.Kind == EDIVEFocusKind::Primitive)
	{
		const UPrimitiveComponent* Primitive = Resolved.Primitive.Get();
		AppendLine(Out, FString::Printf(
			TEXT("    Resolve: Primitive FName='%s' GetName='%s' Normalized='%s' Class=%s PartId='%s'"),
			Primitive ? *Primitive->GetFName().ToString() : TEXT("<null>"),
			Primitive ? *Primitive->GetName() : TEXT("<null>"),
			Primitive ? *DIVE::NormalizeComponentToken(Primitive->GetName()) : TEXT("<null>"),
			Primitive ? *Primitive->GetClass()->GetName() : TEXT("<null>"),
			*Resolved.SemanticPartId.ToString()));
	}
	else if (Resolved.Kind == EDIVEFocusKind::Anchor)
	{
		const USceneComponent* Anchor = Resolved.Anchor.Get();
		AppendLine(Out, FString::Printf(
			TEXT("    Resolve: Anchor FName='%s' PartId='%s'"),
			Anchor ? *Anchor->GetFName().ToString() : TEXT("<null>"),
			*Resolved.SemanticPartId.ToString()));
	}

	for (const FDIVEPickContextMenuAction& Action : ActionList.Actions)
	{
		const FName ResolvedId = DIVEDeviceActionResolve::ResolveActionId(Action);
		const FName IsName = DIVE::MakePickContextMenuActiveStateName(CatalogKey, ResolvedId);
		const FText ResolvedDisplay = DIVEDeviceActionResolve::ResolveDisplayName(Action);
		const bool bToggle = DIVEDeviceActionResolve::ResolveToggleActiveSuffix(Action);

		AppendLine(Out, FString::Printf(
			TEXT("    ActionId='%s' Display='%s' Enabled=%s ToggleSuffix=%s"),
			*ResolvedId.ToString(),
			ResolvedDisplay.IsEmpty() ? TEXT("<empty>") : *ResolvedDisplay.ToString(),
			*YesNo(Action.bEnabled),
			*YesNo(bToggle)));
		if (Action.Definition)
		{
			AppendLine(Out, FString::Printf(
				TEXT("      Definition=%s (%s)"),
				*Action.Definition->GetPathName(),
				*GetNameSafe(Action.Definition->GetClass())));
		}
		else
		{
			AppendLine(Out, TEXT("      Definition=<MISSING>"));
		}
		AppendLine(Out, FString::Printf(
			TEXT("      IsState %s -> property=%s function=%s"),
			*IsName.ToString(),
			*YesNo(HasBoolProperty(Owner, IsName)),
			*YesNo(HasBoolFunction(Owner, IsName))));

		if (HasBoolProperty(Owner, IsName))
		{
			bool bInstance = false;
			bool bCdo = false;
			if (const FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Owner->GetClass(), IsName))
			{
				bInstance = BoolProperty->GetPropertyValue_InContainer(Owner);
				if (const AActor* CDO = Cast<AActor>(Owner->GetClass()->GetDefaultObject()))
				{
					bCdo = BoolProperty->GetPropertyValue_InContainer(CDO);
				}
			}
			AppendLine(Out, FString::Printf(
				TEXT("      IsState values: instance=%s CDO_default=%s%s"),
				*YesNo(bInstance),
				*YesNo(bCdo),
				(bInstance != bCdo) ? TEXT("  <-- instance differs from BP default") : TEXT("")));
		}

		if (bToggle
			&& !HasBoolProperty(Owner, IsName)
			&& !HasBoolFunction(Owner, IsName))
		{
			AppendLine(Out, TEXT("      WARNING: Toggle Active Suffix on, but no Is_* property/function on device actor."));
		}
	}
}

void DumpPrimitiveRow(
	FString& Out,
	UDIVEInspectableComponent* Inspectable,
	UPrimitiveComponent* Primitive)
{
	if (!Primitive || !Inspectable)
	{
		return;
	}

	const FName SemanticPartId = Inspectable->ResolveSemanticPartId(Primitive);
	const FDIVEFocusTarget PickTarget = FDIVEFocusTarget::FromPrimitive(Primitive, SemanticPartId);

	TArray<FDIVEContextMenuEntry> CustomEntries;
	Inspectable->AppendConfiguredPickContextMenuEntries(PickTarget, CustomEntries);

	FString CustomSummary = TEXT("(none)");
	if (!CustomEntries.IsEmpty())
	{
		TArray<FString> Parts;
		for (const FDIVEContextMenuEntry& Entry : CustomEntries)
		{
			Parts.Add(FString::Printf(TEXT("%s['%s']"),
				*Entry.ActionId.ToString(),
				*Entry.DisplayName.ToString()));
		}
		CustomSummary = FString::Join(Parts, TEXT(", "));
	}

	AppendLine(Out, FString::Printf(
		TEXT("  Prim FName='%s' GetName='%s' Normalized='%s' Class=%s Owner=%s"),
		*Primitive->GetFName().ToString(),
		*Primitive->GetName(),
		*DIVE::NormalizeComponentToken(Primitive->GetName()),
		*Primitive->GetClass()->GetName(),
		Primitive->GetOwner() ? *Primitive->GetOwner()->GetName() : TEXT("<none>")));
	AppendLine(Out, FString::Printf(
		TEXT("    HiddenInGame=%s Visible=%s Collision=%d Shape=%s TagPickProxy=%s"),
		*YesNo(Primitive->bHiddenInGame),
		*YesNo(Primitive->IsVisible()),
		static_cast<int32>(Primitive->GetCollisionEnabled()),
		*YesNo(Primitive->IsA(UShapeComponent::StaticClass())),
		*YesNo(!Inspectable->PickProxyComponentTag.IsNone()
			&& Primitive->ComponentHasTag(Inspectable->PickProxyComponentTag))));
	AppendLine(Out, FString::Printf(
		TEXT("    Pickable=%s PickProxy=%s Interactive=%s PartId='%s'"),
		*YesNo(Inspectable->IsPrimitivePickable(Primitive)),
		*YesNo(Inspectable->IsPickProxyPrimitive(Primitive)),
		*YesNo(Inspectable->IsPrimitiveInteractive(Primitive)),
		*SemanticPartId.ToString()));
	AppendLine(Out, FString::Printf(TEXT("    CatalogMatch custom rows: %s"), *CustomSummary));

	if (Inspectable->IsPickProxyPrimitive(Primitive)
		&& Inspectable->IsPrimitiveInteractive(Primitive)
		&& CustomEntries.IsEmpty()
		&& !Inspectable->PickContextMenuByComponent.IsEmpty())
	{
		AppendLine(Out, TEXT("    WARNING: interactive pick-proxy with NO catalog match — custom Toggle will not appear."));
	}
}
} // namespace

FString DIVEDebugDump::BuildDeviceDump(AActor* DeviceActor)
{
	FString Out;
	if (!DeviceActor)
	{
		AppendLine(Out, TEXT("DIVE Dump: <null actor>"));
		return Out;
	}

	UDIVEInspectableComponent* Inspectable = DeviceActor->FindComponentByClass<UDIVEInspectableComponent>();
	AppendLine(Out, TEXT("==== DIVE Device Dump ===="));
	AppendLine(Out, FString::Printf(
		TEXT("Actor='%s' Class=%s Path=%s"),
		*DeviceActor->GetName(),
		*DeviceActor->GetClass()->GetName(),
		*DeviceActor->GetPathName()));

	if (!Inspectable)
	{
		AppendLine(Out, TEXT("ERROR: no UDIVEInspectableComponent on this actor."));
		AppendLine(Out, TEXT("==== end ===="));
		return Out;
	}

	Inspectable->BuildSemanticRegistry();

	AppendLine(Out, FString::Printf(
		TEXT("Inspectable SessionActive=%s CatalogEntries=%d Exclusions=%d"),
		*YesNo(Inspectable->IsSessionActive()),
		Inspectable->PickContextMenuByComponent.Num(),
		Inspectable->PickInteractionExclusions.Num()));

	AppendLine(Out, TEXT("-- Device action dispatch --"));
	AppendLine(Out, FString::Printf(
		TEXT("  IDIVEDeviceActionHandler: %s"),
		*YesNo(DeviceActor->Implements<UDIVEDeviceActionHandler>())));
	if (!DeviceActor->Implements<UDIVEDeviceActionHandler>())
	{
		AppendLine(Out, TEXT("  WARNING: device must implement IDIVEDeviceActionHandler for custom catalog actions."));
	}

	AppendLine(Out, TEXT("-- Catalog (PickContextMenuByComponent) --"));
	if (Inspectable->PickContextMenuByComponent.IsEmpty())
	{
		AppendLine(Out, TEXT("  (empty)"));
	}
	else
	{
		for (const TPair<FName, FDIVEPickContextMenuActionList>& Entry : Inspectable->PickContextMenuByComponent)
		{
			DumpCatalogEntry(Out, DeviceActor, Inspectable, Entry.Key, Entry.Value);
		}
	}

	AppendLine(Out, TEXT("-- Cross-check: CatalogKey vs shape / Switch* FNames --"));
	{
		TArray<UPrimitiveComponent*> Primitives;
		DIVE::CollectDevicePrimitives(DeviceActor, Primitives);

		TMap<FString, TArray<FString>> NormalizedToFNames;
		for (UPrimitiveComponent* Primitive : Primitives)
		{
			if (!Primitive)
			{
				continue;
			}

			if (!Primitive->IsA(UShapeComponent::StaticClass())
				&& !Primitive->GetName().Contains(TEXT("Switch"), ESearchCase::IgnoreCase))
			{
				continue;
			}

			NormalizedToFNames.FindOrAdd(DIVE::NormalizeComponentToken(Primitive->GetName())).Add(Primitive->GetFName().ToString());
		}

		for (const TPair<FName, FDIVEPickContextMenuActionList>& Entry : Inspectable->PickContextMenuByComponent)
		{
			const FString KeyNorm = DIVE::NormalizeComponentToken(Entry.Key.ToString());
			if (const TArray<FString>* Names = NormalizedToFNames.Find(KeyNorm))
			{
				AppendLine(Out, FString::Printf(
					TEXT("  OK  CatalogKey='%s' (norm='%s') -> FName(s): %s | Actions=%d"),
					*Entry.Key.ToString(),
					*KeyNorm,
					*FString::Join(*Names, TEXT(", ")),
					Entry.Value.Actions.Num()));
			}
			else
			{
				AppendLine(Out, FString::Printf(
					TEXT("  FAIL CatalogKey='%s' (norm='%s') — no shape/Switch* primitive with this normalized name | Actions=%d"),
					*Entry.Key.ToString(),
					*KeyNorm,
					Entry.Value.Actions.Num()));
			}
		}

		for (const TPair<FString, TArray<FString>>& NormEntry : NormalizedToFNames)
		{
			bool bHasCatalog = false;
			for (const TPair<FName, FDIVEPickContextMenuActionList>& CatalogEntry : Inspectable->PickContextMenuByComponent)
			{
				if (DIVE::NormalizeComponentToken(CatalogEntry.Key.ToString()).Equals(NormEntry.Key, ESearchCase::IgnoreCase))
				{
					bHasCatalog = true;
					break;
				}
			}

			if (!bHasCatalog)
			{
				AppendLine(Out, FString::Printf(
					TEXT("  FAIL Prim norm='%s' FName(s): %s — no catalog key"),
					*NormEntry.Key,
					*FString::Join(NormEntry.Value, TEXT(", "))));
			}
		}
	}

	AppendLine(Out, TEXT("-- Primitives (shapes, pick-proxies, Switch*, interactive) --"));
	TArray<UPrimitiveComponent*> Primitives;
	DIVE::CollectDevicePrimitives(DeviceActor, Primitives);
	Primitives.Sort([](const UPrimitiveComponent& A, const UPrimitiveComponent& B)
	{
		return A.GetName() < B.GetName();
	});

	int32 ShapeOrProxyCount = 0;
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!Primitive)
		{
			continue;
		}

		const bool bInteresting = Primitive->IsA(UShapeComponent::StaticClass())
			|| Inspectable->IsPickProxyPrimitive(Primitive)
			|| Primitive->GetName().Contains(TEXT("Switch"), ESearchCase::IgnoreCase);

		if (!bInteresting && !Inspectable->IsPrimitiveInteractive(Primitive))
		{
			continue;
		}

		if (Primitive->IsA(UShapeComponent::StaticClass()) || Inspectable->IsPickProxyPrimitive(Primitive))
		{
			++ShapeOrProxyCount;
		}

		DumpPrimitiveRow(Out, Inspectable, Primitive);
	}

	AppendLine(Out, FString::Printf(TEXT("Shape/PickProxy count (listed interesting): %d"), ShapeOrProxyCount));
	AppendLine(Out, TEXT("Hint: CatalogKey must match Prim FName or Normalized (strip _GEN_VARIABLE / _1)."));
	AppendLine(Out, TEXT("==== end ===="));
	return Out;
}

FString DIVEDebugDump::DumpDevice(AActor* DeviceActor)
{
	const FString Body = BuildDeviceDump(DeviceActor);
	UE_LOG(LogDIVE, Display, TEXT("%s"), *Body);
	WriteDumpFile(DeviceActor ? DeviceActor->GetName() : TEXT("Null"), Body);
	return Body;
}

FString DIVEDebugDump::DumpAllInWorld(UWorld* World)
{
	FString Out;
	AppendLine(Out, TEXT("==== DIVE DumpAll ===="));

	if (!World)
	{
		AppendLine(Out, TEXT("ERROR: no world."));
		AppendLine(Out, TEXT("==== end ===="));
		UE_LOG(LogDIVE, Display, TEXT("%s"), *Out);
		return Out;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || !Actor->FindComponentByClass<UDIVEInspectableComponent>())
		{
			continue;
		}

		++Count;
		Out += BuildDeviceDump(Actor);
		AppendLine(Out, TEXT(""));
	}

	AppendLine(Out, FString::Printf(TEXT("Devices with Inspectable: %d"), Count));
	AppendLine(Out, TEXT("==== DumpAll end ===="));
	UE_LOG(LogDIVE, Display, TEXT("%s"), *Out);
	WriteDumpFile(TEXT("All"), Out);
	return Out;
}

FString NotifyDumpFailure(const FString& Message)
{
	UE_LOG(LogDIVE, Warning, TEXT("%s"), *Message);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 6.0f, FColor::Red, Message);
	}
	return Message;
}

bool ActorMatchesDumpFilter(const AActor* Actor, const FString& Filter)
{
	if (!Actor || Filter.IsEmpty())
	{
		return true;
	}

	auto Matches = [](const FString& Candidate, const FString& Needle) -> bool
	{
		return !Candidate.IsEmpty() && !Needle.IsEmpty()
			&& Candidate.Contains(Needle, ESearchCase::IgnoreCase);
	};

	const FString ActorName = Actor->GetName();
	const FString ActorLabel = Actor->GetActorNameOrLabel();
	const FString ClassName = Actor->GetClass()->GetName();
	const FString PathName = Actor->GetPathName();

	if (Matches(ActorName, Filter)
		|| Matches(ActorLabel, Filter)
		|| Matches(ClassName, Filter)
		|| Matches(PathName, Filter))
	{
		return true;
	}

	// Accept "ElectricalPanel" for class BP_ElectricalPanel_C / label BP_ElectricalPanel.
	FString CompactFilter = Filter;
	CompactFilter.ReplaceInline(TEXT("BP_"), TEXT(""), ESearchCase::IgnoreCase);
	if (CompactFilter.EndsWith(TEXT("_C"), ESearchCase::IgnoreCase))
	{
		CompactFilter.LeftChopInline(2);
	}

	if (CompactFilter.IsEmpty() || CompactFilter.Equals(Filter, ESearchCase::IgnoreCase))
	{
		return false;
	}

	return Matches(ActorName, CompactFilter)
		|| Matches(ActorLabel, CompactFilter)
		|| Matches(ClassName, CompactFilter)
		|| Matches(PathName, CompactFilter);
}

FString DIVEDebugDump::DumpFromConsole(UWorld* World, const TArray<FString>& Args, const bool bAll)
{
	if (bAll)
	{
		return DumpAllInWorld(World);
	}

	if (!World)
	{
		return NotifyDumpFailure(TEXT("DIVE.DumpDevice: no world (use in PIE / game)."));
	}

	const FString Filter = Args.Num() > 0 ? Args[0] : FString();

	// Explicit filter: search the world (do not force the active session device if names differ).
	if (!Filter.IsEmpty())
	{
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor || !Actor->FindComponentByClass<UDIVEInspectableComponent>())
			{
				continue;
			}

			if (ActorMatchesDumpFilter(Actor, Filter))
			{
				return DumpDevice(Actor);
			}
		}

		return NotifyDumpFailure(FString::Printf(
			TEXT("DIVE.DumpDevice: no inspectable actor matching '%s'. Try DIVE.DumpAll or a substring of the class (e.g. ElectricBox)."),
			*Filter));
	}

	if (UGameInstance* GameInstance = World->GetGameInstance())
	{
		if (UDIVESessionSubsystem* Session = GameInstance->GetSubsystem<UDIVESessionSubsystem>())
		{
			if (AActor* Active = Session->GetActiveDeviceHost())
			{
				return DumpDevice(Active);
			}
		}
	}

	AActor* Best = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || !Actor->FindComponentByClass<UDIVEInspectableComponent>())
		{
			continue;
		}

		Best = Actor;
		break;
	}

	if (!Best)
	{
		return NotifyDumpFailure(
			TEXT("DIVE.DumpDevice: no actor with UDIVEInspectableComponent (and no active session)."));
	}

	return DumpDevice(Best);
}

void DIVEDebugDump::RegisterConsoleCommands()
{
#if UE_BUILD_SHIPPING
	return;
#else
	UnregisterConsoleCommands();

	auto Bind = [](const TCHAR* Name, const TCHAR* Help, bool bAll)
	{
		IConsoleObject* Obj = IConsoleManager::Get().RegisterConsoleCommand(
			Name,
			Help,
			FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateLambda(
				[bAll](const TArray<FString>& Args, UWorld* World, FOutputDevice& Output)
				{
					const FString Body = DumpFromConsole(World, Args, bAll);
					Output.Log(*Body);
					if (!GLastDumpFilePath.IsEmpty())
					{
						Output.Logf(TEXT("DIVE dump file: %s"), *GLastDumpFilePath);
					}
				}),
			ECVF_Default);
		if (Obj)
		{
			GRegisteredConsoleObjects.Add(Obj);
		}
	};

	Bind(
		TEXT("DIVE.DumpDevice"),
		TEXT("Dump DIVE inspectable catalog vs component FNames. Optional arg: actor name substring. Prefers active session device."),
		false);
	Bind(
		TEXT("DIVE_DumpDevice"),
		TEXT("Alias of DIVE.DumpDevice."),
		false);
	Bind(
		TEXT("DIVE.DumpAll"),
		TEXT("Dump every actor with UDIVEInspectableComponent in the world."),
		true);
	Bind(
		TEXT("DIVE_DumpAll"),
		TEXT("Alias of DIVE.DumpAll."),
		true);
#endif
}

void DIVEDebugDump::UnregisterConsoleCommands()
{
	for (IConsoleObject* Obj : GRegisteredConsoleObjects)
	{
		if (Obj)
		{
			IConsoleManager::Get().UnregisterConsoleObject(Obj);
		}
	}
	GRegisteredConsoleObjects.Reset();
}

FString DIVEDebugDump::GetLastDumpFilePath()
{
	return GLastDumpFilePath;
}
