// Copyright (c) 2026. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Actions/DIVEBuiltInActions.h"
#include "DIVEActionBinding.h"
#include "DIVEConvention.h"
#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVETypes.h"
#include "Utils/DIVEContextMenu.h"

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "UObject/Package.h"

namespace
{
constexpr EAutomationTestFlags SmokeUnitTestFlags =
	EAutomationTestFlags::EditorContext
	| EAutomationTestFlags::ClientContext
	| EAutomationTestFlags::SmokeFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEContextMenuDefaultBindingsSmokeTest,
	"DIVE.ContextMenu.DefaultBindings",
	SmokeUnitTestFlags)

bool FDIVEContextMenuDefaultBindingsSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FDIVEContextMenuEntry> Entries;
	const FDIVEFocusTarget InvalidPick = FDIVEFocusTarget::FromPrimitive(nullptr, NAME_None);

	DIVEContextMenu::BuildEntries(nullptr, InvalidPick, false, Entries);
	TestEqual(TEXT("No inspectable => empty menu"), Entries.Num(), 0);

	// Use CDO — NewObject<UActorComponent>() at Smoke startup runs before EngineElements
	// registers Typed Element type "Components" and asserts in PostInitProperties.
	const UDIVEInspectableComponent* Inspectable =
		UDIVEInspectableComponent::StaticClass()->GetDefaultObject<UDIVEInspectableComponent>();
	TestNotNull(TEXT("Inspectable CDO available"), Inspectable);
	if (!Inspectable)
	{
		return false;
	}
	TestTrue(TEXT("CDO-seeded Bindings present"), Inspectable->Bindings.Num() >= 1);

	bool bFoundFocus = false;
	bool bAnyPrimitiveBinding = false;
	for (const FDIVEActionBinding& Binding : Inspectable->Bindings)
	{
		if (Binding.Targets.MatchMode == EDIVETargetMatchMode::AnyPrimitive)
		{
			bAnyPrimitiveBinding = true;
		}
		for (UDIVEDeviceAction* Action : Binding.Actions)
		{
			if (Action && Action->IsA<UDIVEFocusAction>())
			{
				bFoundFocus = true;
			}
		}
	}
	TestTrue(TEXT("Default Bindings include Focus"), bFoundFocus);
	TestTrue(TEXT("Default Bindings use AnyPrimitive"), bAnyPrimitiveBinding);

	FDIVEFocusTarget DummyPick = FDIVEFocusTarget::FromPrimitive(nullptr, NAME_None);
	TestFalse(TEXT("AnyPrimitive does not match invalid primitive pick"),
		Inspectable->DoesTargetQueryMatchPick(Inspectable->Bindings[0].Targets, DummyPick));

	TArray<FDIVEMenuSection> Sections;
	Inspectable->GatherAuthoredSections(Sections);
	bool bFoundAdminHeader = false;
	for (const FDIVEMenuSection& Section : Sections)
	{
		if (Section.SectionId == DIVE::kSectionAdmin && !Section.Header.IsEmpty())
		{
			bFoundAdminHeader = true;
		}
	}
	TestTrue(TEXT("Default Admin section has Header"), bFoundAdminHeader);

	TestTrue(TEXT("AnyPrimitive is a distinct match mode"),
		EDIVETargetMatchMode::AnyPrimitive != EDIVETargetMatchMode::ComponentTag);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEPawnPhysicalDriveResolveSmokeTest,
	"DIVE.PawnPhysicalDrive.Resolve",
	SmokeUnitTestFlags)

bool FDIVEPawnPhysicalDriveResolveSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(TEXT("FindOnPawn null for null pawn"), DIVEPawnPhysicalDriveResolve::FindOnPawn(nullptr));
	TestNull(TEXT("FindOnPlayerController null for null PC"), DIVEPawnPhysicalDriveResolve::FindOnPlayerController(nullptr));
	TestNull(TEXT("FindProxyDriveForHit null for null component"), DIVEProxyDriveResolve::FindProxyDriveForHit(nullptr));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEActionsBindingResolveSmokeTest,
	"DIVE.Actions.BindingResolve",
	SmokeUnitTestFlags)

bool FDIVEActionsBindingResolveSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	FDIVEActionBinding Binding;
	Binding.BindingId = TEXT("Bolts");
	Binding.Targets.MatchMode = EDIVETargetMatchMode::ComponentTag;
	Binding.Targets.MatchValues = { TEXT("DIVE.Bolt") };
	Binding.Targets.Priority = 10;
	Binding.SectionId = DIVE::kSectionStandard;
	Binding.PrimaryActionIndex = 0;

	TestEqual(TEXT("PrimaryAction null when Actions empty"), Binding.GetPrimaryAction(), static_cast<UDIVEDeviceAction*>(nullptr));

	UDIVEFocusAction* Focus = NewObject<UDIVEFocusAction>();
	Binding.Actions.Add(Focus);
	TestEqual(TEXT("PrimaryAction resolves index 0"), Binding.GetPrimaryAction(), static_cast<UDIVEDeviceAction*>(Focus));

	Binding.PrimaryActionIndex = 5;
	TestNull(TEXT("PrimaryAction null for invalid index"), Binding.GetPrimaryAction());

	TestTrue(TEXT("Standard section id stable"), DIVE::kSectionStandard == FName(TEXT("Standard")));
	TestTrue(TEXT("Admin section id stable"), DIVE::kSectionAdmin == FName(TEXT("Admin")));

	UDIVEContinuousDeviceAction* Continuous = NewObject<UDIVEProxyDriveForwardAction>();
	TestNotNull(TEXT("Continuous action instance"), Continuous);
	if (Continuous)
	{
		TestFalse(TEXT("Continuous inactive by default"), Continuous->IsInteractionActive());
		Continuous->MarkInteractionActive();
		TestTrue(TEXT("MarkInteractionActive sets flag"), Continuous->IsInteractionActive());
		Continuous->NotifyInteractionCompleted();
		TestFalse(TEXT("NotifyInteractionCompleted clears flag"), Continuous->IsInteractionActive());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEActionWorldContextSmokeTest,
	"DIVE.Actions.ExecutionWorld",
	SmokeUnitTestFlags)

bool FDIVEActionWorldContextSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// Mimic a catalog-hosted action: Outer is a package, not a world-bound object.
	UDIVEFocusAction* Action = NewObject<UDIVEFocusAction>(GetTransientPackage());
	TestNotNull(TEXT("Action instance"), Action);
	if (!Action)
	{
		return false;
	}

	TestNull(TEXT("Package-outered action has no world by default"), Action->GetWorld());

	// Do NOT CreateWorld/DestroyWorld here — Inactive CreateWorld crashes in Editor smoke
	// without a full world init path. Prefer an already-live Engine world when available.
	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (UWorld* Candidate = Context.World())
			{
				World = Candidate;
				break;
			}
		}
	}

	if (World)
	{
		{
			FDIVEActionWorldScope Scope(Action, World);
			TestEqual(TEXT("ExecutionWorld injected via scope"), Action->GetWorld(), World);
		}
		TestNull(TEXT("ExecutionWorld cleared after scope"), Action->GetWorld());
	}
	else
	{
		// Still verify RAII clear path without a live world pointer.
		{
			FDIVEActionWorldScope Scope(Action, nullptr);
			TestNull(TEXT("Scope with null world keeps GetWorld null"), Action->GetWorld());
		}
		TestNull(TEXT("ExecutionWorld still null after null scope"), Action->GetWorld());
		AddInfo(TEXT("No live Engine world — skipped non-null ExecutionWorld injection assert."));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEActionCanExecuteGateSmokeTest,
	"DIVE.Actions.CanExecuteGate",
	SmokeUnitTestFlags)

bool FDIVEActionCanExecuteGateSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UDIVEFocusAction* Focus = NewObject<UDIVEFocusAction>();
	TestNotNull(TEXT("Focus action"), Focus);
	if (!Focus)
	{
		return false;
	}

	FDIVEActionContext EmptyContext;
	TestFalse(
		TEXT("Focus CanExecute is false without a valid primitive pick"),
		Focus->CanExecute(EmptyContext));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
