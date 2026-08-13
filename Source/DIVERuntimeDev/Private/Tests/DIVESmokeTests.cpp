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

	// CDO, not NewObject — instance UActorComponent asserts in PostInitProperties until
	// EngineElements registers Typed Element type "Components" (see DefaultBindings smoke).
	const UDIVEInspectableComponent* Inspectable =
		UDIVEInspectableComponent::StaticClass()->GetDefaultObject<UDIVEInspectableComponent>();
	TestNotNull(TEXT("Inspectable CDO for MakeActionContext"), Inspectable);
	if (Inspectable)
	{
		const FDIVEFocusTarget Pick = FDIVEFocusTarget::FromPrimitive(nullptr, NAME_None);
		const FDIVEActionContext Context = Inspectable->MakeActionContext(
			Pick,
			NAME_None,
			FVector2D::ZeroVector,
			FHitResult(),
			FName(TEXT("CoverBolts")));
		TestEqual(
			TEXT("MakeActionContext copies BindingId"),
			Context.BindingId,
			FName(TEXT("CoverBolts")));
		const FDIVEActionContext EmptyBindingContext = Inspectable->MakeActionContext(Pick, NAME_None);
		TestTrue(TEXT("MakeActionContext BindingId defaults to None"), EmptyBindingContext.BindingId.IsNone());
	}

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

	UDIVENotifyAction* Notify = NewObject<UDIVENotifyAction>();
	TestNotNull(TEXT("Notify action instance"), Notify);
	if (Notify)
	{
		const FDIVEActionContext EmptyContext;
		TestTrue(TEXT("Notify CanExecute is true without a pick"), Notify->CanExecute(EmptyContext));
		TestTrue(TEXT("Notify Execute succeeds with no extra logic"), Notify->Execute(EmptyContext));
	}

	// LMB primary: specificity beats authored order among unequal modes.
	{
		UDIVENotifyAction* NamePrimary = NewObject<UDIVENotifyAction>();
		UDIVEFocusAction* AnyPrimary = NewObject<UDIVEFocusAction>();
		TestNotNull(TEXT("Name primary action"), NamePrimary);
		TestNotNull(TEXT("Any primary action"), AnyPrimary);
		if (NamePrimary && AnyPrimary)
		{
			FDIVEActionBinding AnyBinding;
			AnyBinding.BindingId = TEXT("BuiltIn.Standard");
			AnyBinding.Targets.MatchMode = EDIVETargetMatchMode::AnyPrimitive;
			AnyBinding.Actions.Add(AnyPrimary);
			AnyBinding.PrimaryActionIndex = 0;

			FDIVEActionBinding NameBinding;
			NameBinding.BindingId = TEXT("Switches");
			NameBinding.Targets.MatchMode = EDIVETargetMatchMode::ComponentName;
			NameBinding.Targets.MatchValues = { TEXT("Switch1") };
			NameBinding.Actions.Add(NamePrimary);
			NameBinding.PrimaryActionIndex = 0;

			TArray<const FDIVEActionBinding*> Matched;
			Matched.Add(&AnyBinding);
			Matched.Add(&NameBinding);

			const FDIVEActionBinding* Winner = SelectPrimaryBinding(Matched);
			TestTrue(TEXT("ComponentName primary beats AnyPrimitive"), Winner == &NameBinding);
			TestEqual(
				TEXT("Specificity Name > Any"),
				GetTargetMatchSpecificity(EDIVETargetMatchMode::ComponentName),
				3);
			TestEqual(
				TEXT("Specificity Any is 0"),
				GetTargetMatchSpecificity(EDIVETargetMatchMode::AnyPrimitive),
				0);
		}
	}

	// Equal specificity: first in Matched (authored order) wins.
	{
		UDIVENotifyAction* FirstAction = NewObject<UDIVENotifyAction>();
		UDIVENotifyAction* SecondAction = NewObject<UDIVENotifyAction>();
		TestNotNull(TEXT("Equal-spec first action"), FirstAction);
		TestNotNull(TEXT("Equal-spec second action"), SecondAction);
		if (FirstAction && SecondAction)
		{
			FDIVEActionBinding First;
			First.BindingId = TEXT("SwitchA");
			First.Targets.MatchMode = EDIVETargetMatchMode::ComponentName;
			First.Actions.Add(FirstAction);
			First.PrimaryActionIndex = 0;

			FDIVEActionBinding Second;
			Second.BindingId = TEXT("SwitchB");
			Second.Targets.MatchMode = EDIVETargetMatchMode::ComponentName;
			Second.Actions.Add(SecondAction);
			Second.PrimaryActionIndex = 0;

			TArray<const FDIVEActionBinding*> Matched;
			Matched.Add(&First);
			Matched.Add(&Second);

			const FDIVEActionBinding* Winner = SelectPrimaryBinding(Matched);
			TestTrue(TEXT("Equal Name specificity keeps authored order"), Winner == &First);
		}
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
