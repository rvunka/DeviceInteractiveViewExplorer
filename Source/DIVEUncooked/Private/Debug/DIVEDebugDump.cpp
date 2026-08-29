// Copyright (c) 2026. All Rights Reserved.

#include "Debug/DIVEDebugDump.h"

#include "DIVEActionBinding.h"
#include "DIVEActionCatalogAsset.h"
#include "DIVEConvention.h"
#include "DIVEDeviceAction.h"
#include "DIVEHierarchy.h"
#include "DIVEInspectableComponent.h"
#include "DIVELog.h"
#include "DIVESessionSubsystem.h"
#include "DIVETypes.h"

#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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

void DumpBinding(FString& Out, const FDIVEActionBinding& Binding, const TCHAR* SourceTag)
{
	const TCHAR* MatchModeName = TEXT("?");
	switch (Binding.Targets.MatchMode)
	{
	case EDIVETargetMatchMode::ComponentTag: MatchModeName = TEXT("ComponentTag"); break;
	case EDIVETargetMatchMode::ComponentName: MatchModeName = TEXT("ComponentName"); break;
	case EDIVETargetMatchMode::PartId: MatchModeName = TEXT("PartId"); break;
	case EDIVETargetMatchMode::AnyPrimitive: MatchModeName = TEXT("AnyPrimitive"); break;
	default: break;
	}

	TArray<FString> ValueStrings;
	for (const FName Value : Binding.Targets.MatchValues)
	{
		ValueStrings.Add(Value.ToString());
	}

	AppendLine(Out, FString::Printf(
		TEXT("  [%s] BindingId='%s' Match=%s Values=[%s] Section='%s' PrimaryIndex=%d Actions=%d"),
		SourceTag,
		*Binding.BindingId.ToString(),
		MatchModeName,
		*FString::Join(ValueStrings, TEXT(", ")),
		*Binding.SectionId.ToString(),
		Binding.PrimaryActionIndex,
		Binding.Actions.Num()));

	for (int32 ActionIndex = 0; ActionIndex < Binding.Actions.Num(); ++ActionIndex)
	{
		UDIVEDeviceAction* Action = Binding.Actions[ActionIndex];
		if (!Action)
		{
			AppendLine(Out, FString::Printf(TEXT("    [%d] <null>"), ActionIndex));
			continue;
		}

		AppendLine(Out, FString::Printf(
			TEXT("    [%d] Class=%s Display='%s'%s"),
			ActionIndex,
			*Action->GetClass()->GetName(),
			*Action->GetResolvedDisplayName().ToString(),
			Binding.PrimaryActionIndex == ActionIndex ? TEXT(" [PRIMARY]") : TEXT("")));
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

	TArray<FDIVEContextMenuEntry> MenuEntries;
	Inspectable->AppendConfiguredContextMenuEntries(PickTarget, MenuEntries);

	FString MenuSummary = TEXT("(none)");
	if (!MenuEntries.IsEmpty())
	{
		TArray<FString> Parts;
		for (const FDIVEContextMenuEntry& Entry : MenuEntries)
		{
			if (Entry.bIsSeparator)
			{
				Parts.Add(Entry.DisplayName.IsEmpty()
					? TEXT("---")
					: FString::Printf(TEXT("---[%s]"), *Entry.DisplayName.ToString()));
				continue;
			}

			Parts.Add(FString::Printf(
				TEXT("%s['%s'%s]"),
				Entry.Action ? *Entry.Action->GetClass()->GetName() : TEXT("<null>"),
				*Entry.DisplayName.ToString(),
				Entry.TargetKey.IsNone()
					? TEXT("")
					: *FString::Printf(TEXT(" key=%s"), *Entry.TargetKey.ToString())));
		}
		MenuSummary = FString::Join(Parts, TEXT(", "));
	}

	TArray<FString> TagStrings;
	for (const FName Tag : Primitive->ComponentTags)
	{
		TagStrings.Add(Tag.ToString());
	}

	AppendLine(Out, FString::Printf(
		TEXT("  Prim FName='%s' GetName='%s' Normalized='%s' Class=%s Owner=%s"),
		*Primitive->GetFName().ToString(),
		*Primitive->GetName(),
		*DIVE::NormalizeComponentToken(Primitive->GetName()),
		*Primitive->GetClass()->GetName(),
		Primitive->GetOwner() ? *Primitive->GetOwner()->GetName() : TEXT("<none>")));
	AppendLine(Out, FString::Printf(
		TEXT("    HiddenInGame=%s Visible=%s Collision=%d Shape=%s TagPickProxy=%s Tags=[%s]"),
		*YesNo(Primitive->bHiddenInGame),
		*YesNo(Primitive->IsVisible()),
		static_cast<int32>(Primitive->GetCollisionEnabled()),
		*YesNo(Primitive->IsA(UShapeComponent::StaticClass())),
		*YesNo(!Inspectable->PickProxyComponentTag.IsNone()
			&& Primitive->ComponentHasTag(Inspectable->PickProxyComponentTag)),
		*FString::Join(TagStrings, TEXT(", "))));
	AppendLine(Out, FString::Printf(
		TEXT("    Pickable=%s PickProxy=%s Interactive=%s PartId='%s'"),
		*YesNo(Inspectable->IsPrimitivePickable(Primitive)),
		*YesNo(Inspectable->IsPickProxyPrimitive(Primitive)),
		*YesNo(Inspectable->IsPrimitiveInteractive(Primitive)),
		*SemanticPartId.ToString()));
	UDIVEDeviceAction* LmbAction = nullptr;
	FName LmbTargetKey = NAME_None;
	FName LmbBindingId = NAME_None;
	FString LmbSummary = TEXT("(none)");
	if (Inspectable->TryResolvePrimaryAction(PickTarget, LmbAction, LmbTargetKey, LmbBindingId) && LmbAction)
	{
		LmbSummary = FString::Printf(
			TEXT("BindingId='%s' Action=%s key=%s"),
			LmbBindingId.IsNone() ? TEXT("<unnamed>") : *LmbBindingId.ToString(),
			*LmbAction->GetClass()->GetName(),
			LmbTargetKey.IsNone() ? TEXT("-") : *LmbTargetKey.ToString());
	}

	AppendLine(Out, FString::Printf(TEXT("    Menu rows (Bindings+Catalog): %s"), *MenuSummary));
	AppendLine(Out, FString::Printf(TEXT("    LMB primary: %s"), *LmbSummary));

	TArray<const FDIVEActionBinding*> Matched;
	Inspectable->GatherMatchingBindings(PickTarget, Matched);

	// Default AnyPrimitive bindings always match; warn when a pick-proxy has catalog
	// authored but no catalog copy matched (only component Bindings did).
	bool bHasCatalogMatch = false;
	for (const FDIVEActionBinding* Binding : Matched)
	{
		if (!Binding)
		{
			continue;
		}

		bool bFromLocalBindings = false;
		for (const FDIVEActionBinding& Local : Inspectable->Bindings)
		{
			if (&Local == Binding)
			{
				bFromLocalBindings = true;
				break;
			}
		}
		if (!bFromLocalBindings && Inspectable->ActionCatalog)
		{
			bHasCatalogMatch = true;
			break;
		}
	}

	const bool bHasCatalogAuthored =
		Inspectable->ActionCatalog != nullptr
		&& Inspectable->ActionCatalog->Bindings.Num() > 0;
	if (Inspectable->IsPickProxyPrimitive(Primitive)
		&& Inspectable->IsPrimitiveInteractive(Primitive)
		&& bHasCatalogAuthored
		&& !bHasCatalogMatch)
	{
		AppendLine(Out, TEXT("    WARNING: interactive pick-proxy matches no Catalog binding — check MatchMode / MatchValues."));
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

	TArray<FDIVEMenuSection> Sections;
	Inspectable->GatherAuthoredSections(Sections);

	TArray<const FDIVEActionBinding*> AuthoredBindings;
	Inspectable->GatherAuthoredBindings(AuthoredBindings);

	AppendLine(Out, FString::Printf(
		TEXT("Inspectable SessionActive=%s Catalog=%s ComponentBindings=%d AuthoredBindings=%d Sections=%d Exclusions=%d"),
		*YesNo(Inspectable->IsSessionActive()),
		Inspectable->ActionCatalog ? *GetNameSafe(Inspectable->ActionCatalog) : TEXT("<none>"),
		Inspectable->Bindings.Num(),
		AuthoredBindings.Num(),
		Sections.Num(),
		Inspectable->PickInteractionExclusions.Num()));

	AppendLine(Out, TEXT("-- Sections --"));
	if (Sections.IsEmpty())
	{
		AppendLine(Out, TEXT("  (empty)"));
	}
	else
	{
		for (const FDIVEMenuSection& Section : Sections)
		{
			AppendLine(Out, FString::Printf(
				TEXT("  SectionId='%s' Header='%s'"),
				*Section.SectionId.ToString(),
				*Section.Header.ToString()));
		}
	}

	AppendLine(Out, TEXT("-- Bindings --"));
	const bool bNoBindings = Inspectable->Bindings.IsEmpty()
		&& (!Inspectable->ActionCatalog || Inspectable->ActionCatalog->Bindings.IsEmpty());
	if (bNoBindings)
	{
		AppendLine(Out, TEXT("  (empty)"));
	}
	else
	{
		for (const FDIVEActionBinding& Binding : Inspectable->Bindings)
		{
			DumpBinding(Out, Binding, TEXT("Component"));
		}

		bool bDumpedCatalogCopy = false;
		for (const FDIVEActionBinding* Binding : AuthoredBindings)
		{
			if (!Binding)
			{
				continue;
			}

			bool bFromLocalBindings = false;
			for (const FDIVEActionBinding& Local : Inspectable->Bindings)
			{
				if (&Local == Binding)
				{
					bFromLocalBindings = true;
					break;
				}
			}
			if (!bFromLocalBindings)
			{
				DumpBinding(Out, *Binding, TEXT("Catalog copy"));
				bDumpedCatalogCopy = true;
			}
		}

		if (!bDumpedCatalogCopy && Inspectable->ActionCatalog)
		{
			for (const FDIVEActionBinding& Binding : Inspectable->ActionCatalog->Bindings)
			{
				DumpBinding(Out, Binding, TEXT("Catalog template"));
			}
		}
	}

	AppendLine(Out, TEXT("-- Resolved bindings (interactive primitives) --"));
	{
		TArray<UPrimitiveComponent*> AllPrimitives;
		DIVE::CollectDevicePrimitives(DeviceActor, AllPrimitives);
		AllPrimitives.Sort([](const UPrimitiveComponent& A, const UPrimitiveComponent& B)
		{
			return A.GetName() < B.GetName();
		});

		int32 ResolvedCount = 0;
		for (UPrimitiveComponent* Primitive : AllPrimitives)
		{
			if (!Primitive || !Inspectable->IsPrimitiveInteractive(Primitive))
			{
				continue;
			}

			const FName SemanticPartId = Inspectable->ResolveSemanticPartId(Primitive);
			const FDIVEFocusTarget PickTarget = FDIVEFocusTarget::FromPrimitive(Primitive, SemanticPartId);
			TArray<const FDIVEActionBinding*> Matched;
			Inspectable->GatherMatchingBindings(PickTarget, Matched);
			if (Matched.IsEmpty())
			{
				continue;
			}

			++ResolvedCount;
			TArray<FString> BindingLabels;
			for (const FDIVEActionBinding* Binding : Matched)
			{
				BindingLabels.Add(Binding && !Binding->BindingId.IsNone()
					? Binding->BindingId.ToString()
					: TEXT("<unnamed>"));
			}
			AppendLine(Out, FString::Printf(
				TEXT("  Prim='%s' Bindings=[%s]"),
				*Primitive->GetName(),
				*FString::Join(BindingLabels, TEXT(", "))));
		}

		if (ResolvedCount == 0)
		{
			AppendLine(Out, TEXT("  (none)"));
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
	AppendLine(Out, TEXT("Hint: Match modes = ComponentTag / ComponentName / PartId / AnyPrimitive. Matching bindings are unioned by section."));
	AppendLine(Out, TEXT("Hint: Menu/section order = Bindings and Sections array order (component, then catalog)."));
	AppendLine(Out, TEXT("Hint: LMB primary = most specific matching binding with PrimaryActionIndex, or a binding whose only action is continuous (Name > PartId > Tag > Any); equal specificity keeps the earlier binding."));
	AppendLine(Out, TEXT("Hint: Menu rows include component Bindings + Action Catalog copies (asset inners are templates, not executed)."));
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

	if (UDIVESessionSubsystem* Session = World->GetSubsystem<UDIVESessionSubsystem>())
	{
		if (AActor* Active = Session->GetActiveDeviceHost())
		{
			return DumpDevice(Active);
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
		TEXT("Dump DIVE Bindings + Catalog and resolved menu rows per primitive. Optional actor name substring. Prefers active session device."),
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
