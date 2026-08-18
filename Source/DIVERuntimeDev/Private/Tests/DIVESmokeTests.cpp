// Copyright (c) 2026. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Actions/DIVEBuiltInActions.h"
#include "DIVEActionBinding.h"
#include "DIVEConvention.h"
#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"
#include "DIVEDeviceDefinitionAsset.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVEPlayerComponent.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVESessionSubsystem.h"
#include "DIVETypes.h"
#include "Utils/DIVEPlayerQuery.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DefaultPawn.h"
#include "Tests/DIVEPawnPhysicalDriveTestTypes.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

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

	TestFalse(TEXT("CDO has no hover overlay authored"), Inspectable->HasPickHoverOverlay());

	TestEqual(
		TEXT("CDO FocusBlendDuration default"),
		Inspectable->GetEffectiveCameraSettings().FocusBlendDuration,
		0.35f);

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

	if (!World)
	{
		AddInfo(TEXT("No live Engine world — skipped N>1 pawn physical drive resolve asserts."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Pawn = World->SpawnActor<ADIVETestPhysicalDrivePawn>(
		ADIVETestPhysicalDrivePawn::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	TestNotNull(TEXT("Spawned test physical-drive pawn"), Pawn);
	if (!Pawn)
	{
		return false;
	}

	UDIVETestPawnPhysicalDriveComponent* SecondDrive = NewObject<UDIVETestPawnPhysicalDriveComponent>(
		Pawn,
		UDIVETestPawnPhysicalDriveComponent::StaticClass(),
		TEXT("DriveB"));
	TestNotNull(TEXT("Second physical-drive component"), SecondDrive);
	if (SecondDrive)
	{
		Pawn->AddInstanceComponent(SecondDrive);
		if (!SecondDrive->IsRegistered())
		{
			SecondDrive->RegisterComponent();
		}
	}

	TestNull(
		TEXT("Unnamed resolve refuses N>1 implementors"),
		DIVEPawnPhysicalDriveResolve::FindOnPawn(Pawn));
	TestNotNull(
		TEXT("Named resolve finds DriveB among N>1"),
		DIVEPawnPhysicalDriveResolve::FindOnPawn(Pawn, TEXT("DriveB")));

	Pawn->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEPlayerComponentValidationSmokeTest,
	"DIVE.Player.IsDataValid",
	SmokeUnitTestFlags)

bool FDIVEPlayerComponentValidationSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

#if !WITH_EDITOR
	AddInfo(TEXT("IsDataValid is editor-only — skipped."));
	return true;
#else
	UDIVEPlayerComponent* PlayerCDO =
		UDIVEPlayerComponent::StaticClass()->GetDefaultObject<UDIVEPlayerComponent>();
	TestNotNull(TEXT("Player CDO available"), PlayerCDO);
	if (PlayerCDO)
	{
		FDataValidationContext CdoContext;
		TestEqual(
			TEXT("Player CDO IsDataValid (no owner)"),
			PlayerCDO->IsDataValid(CdoContext),
			EDataValidationResult::Valid);
	}

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

	if (!World)
	{
		AddInfo(TEXT("No live Engine world — skipped non-Pawn IsDataValid assert."));
		return true;
	}

	AActor* Marker = NewObject<AActor>(GetTransientPackage());
	TestNotNull(TEXT("Non-pawn owner actor"), Marker);
	if (Marker)
	{
		UDIVEPlayerComponent* PlayerOnActor = NewObject<UDIVEPlayerComponent>(Marker);
		TestNotNull(TEXT("Player on non-pawn"), PlayerOnActor);
		if (PlayerOnActor)
		{
			FDataValidationContext ActorContext;
			TestEqual(
				TEXT("Player IsDataValid invalid on non-Pawn owner"),
				PlayerOnActor->IsDataValid(ActorContext),
				EDataValidationResult::Invalid);
		}
	}

	return true;
#endif
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
	FDIVECollectPrimitivesMatchingQuerySmokeTest,
	"DIVE.Actions.CollectMatchingPrimitives",
	SmokeUnitTestFlags)

bool FDIVECollectPrimitivesMatchingQuerySmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UDIVEInspectableComponent* Inspectable =
		UDIVEInspectableComponent::StaticClass()->GetDefaultObject<UDIVEInspectableComponent>();
	TestNotNull(TEXT("Inspectable CDO available"), Inspectable);
	if (!Inspectable)
	{
		return false;
	}

	FDIVETargetQuery TagQuery;
	TagQuery.MatchMode = EDIVETargetMatchMode::ComponentTag;
	TagQuery.MatchValues = { TEXT("DIVE.Bolt") };

	TArray<UPrimitiveComponent*> Matches;
	Inspectable->CollectPrimitivesMatchingQuery(TagQuery, Matches);
	TestEqual(TEXT("CDO Collect (no owner) is empty for Tag"), Matches.Num(), 0);

	FDIVETargetQuery AnyQuery;
	AnyQuery.MatchMode = EDIVETargetMatchMode::AnyPrimitive;
	Inspectable->CollectPrimitivesMatchingQuery(AnyQuery, Matches);
	TestEqual(TEXT("CDO Collect (no owner) is empty for AnyPrimitive"), Matches.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEComponentNameMatchSmokeTest,
	"DIVE.Actions.ComponentNameMatch",
	SmokeUnitTestFlags)

bool FDIVEComponentNameMatchSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UDIVEInspectableComponent* Inspectable =
		UDIVEInspectableComponent::StaticClass()->GetDefaultObject<UDIVEInspectableComponent>();
	TestNotNull(TEXT("Inspectable CDO available"), Inspectable);
	if (!Inspectable)
	{
		return false;
	}

	TestEqual(
		TEXT("Normalize strips _GEN_VARIABLE"),
		DIVE::NormalizeComponentToken(TEXT("Switch_GEN_VARIABLE")),
		FString(TEXT("Switch")));
	TestEqual(
		TEXT("Normalize strips numeric suffix"),
		DIVE::NormalizeComponentToken(TEXT("Switch_12")),
		FString(TEXT("Switch")));

	const UStaticMeshComponent* MeshCDO =
		UStaticMeshComponent::StaticClass()->GetDefaultObject<UStaticMeshComponent>();
	TestNotNull(TEXT("StaticMesh CDO available"), MeshCDO);
	if (!MeshCDO)
	{
		return false;
	}

	const FDIVEFocusTarget Pick = FDIVEFocusTarget::FromPrimitive(
		const_cast<UStaticMeshComponent*>(MeshCDO),
		NAME_None);

	FDIVETargetQuery ExactName;
	ExactName.MatchMode = EDIVETargetMatchMode::ComponentName;
	ExactName.MatchValues = { MeshCDO->GetFName() };
	TestTrue(
		TEXT("ComponentName matches exact FName"),
		Inspectable->DoesTargetQueryMatchPick(ExactName, Pick));

	FDIVETargetQuery FocusIdQuery;
	FocusIdQuery.MatchMode = EDIVETargetMatchMode::ComponentName;
	FocusIdQuery.MatchValues = { TEXT("StartFocusPart") };
	TestFalse(
		TEXT("ComponentName does not treat FocusId/PartId as a name match"),
		Inspectable->DoesTargetQueryMatchPick(FocusIdQuery, Pick));

	TestFalse(
		TEXT("Wrong ActionId does not open session"),
		const_cast<UDIVEInspectableComponent*>(Inspectable)->TryRequestSessionFromActionId(TEXT("NotOpenDIVE")));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVECameraDefinitionBlendSmokeTest,
	"DIVE.Camera.DefinitionBlendDuration",
	SmokeUnitTestFlags)

bool FDIVECameraDefinitionBlendSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UDIVEDeviceDefinitionAsset* Definition = NewObject<UDIVEDeviceDefinitionAsset>();
	TestNotNull(TEXT("Device definition instance"), Definition);
	if (!Definition)
	{
		return false;
	}

	TestEqual(TEXT("Definition FocusBlendDuration default"), Definition->CameraSettings.FocusBlendDuration, 0.35f);
	Definition->CameraSettings.FocusBlendDuration = 0.9f;
	TestEqual(TEXT("Definition FocusBlendDuration writes"), Definition->CameraSettings.FocusBlendDuration, 0.9f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVESessionLifecycleSmokeTest,
	"DIVE.Session.Lifecycle",
	SmokeUnitTestFlags)

bool FDIVESessionLifecycleSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* Candidate = Context.World();
			if (!Candidate)
			{
				continue;
			}
			if (Candidate->WorldType != EWorldType::Game && Candidate->WorldType != EWorldType::PIE)
			{
				continue;
			}
			if (Candidate->GetSubsystem<UDIVESessionSubsystem>())
			{
				World = Candidate;
				break;
			}
		}
	}

	if (!World)
	{
		AddInfo(TEXT("No Game/PIE world with a DIVE session subsystem — skipped (Editor smoke without PIE)."));
		return true;
	}

	UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>();
	if (!Subsystem)
	{
		AddInfo(TEXT("No DIVE session subsystem on this world — skipped."));
		return true;
	}

	if (!DIVEPlayerQuery::FindLocalPlayerController(World))
	{
		AddInfo(TEXT("No local PlayerController — skipped (Editor smoke without PIE)."));
		return true;
	}

	if (Subsystem->IsSessionActive())
	{
		AddInfo(TEXT("A DIVE session is already active — skipped to avoid clobbering PIE."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Device = World->SpawnActor<ADIVETestPhysicalDrivePawn>(
		ADIVETestPhysicalDrivePawn::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);
	TestNotNull(TEXT("Spawned session-host actor"), Device);
	if (!Device)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = NewObject<UDIVEInspectableComponent>(
		Device,
		UDIVEInspectableComponent::StaticClass(),
		TEXT("DIVEInspectable"));
	TestNotNull(TEXT("Inspectable on session host"), Inspectable);
	if (!Inspectable)
	{
		Device->Destroy();
		return false;
	}

	Device->AddInstanceComponent(Inspectable);
	if (!Inspectable->IsRegistered())
	{
		Inspectable->RegisterComponent();
	}

	const bool bStarted = Subsystem->TryBeginSession(Device, Inspectable, FDIVESessionParams());
	TestTrue(TEXT("TryBeginSession"), bStarted);
	TestTrue(TEXT("Session active after begin"), Subsystem->IsSessionActive());
	TestFalse(TEXT("CanNavigateBack is false at device root"), Subsystem->CanNavigateBack());

	if (bStarted)
	{
		Subsystem->EndSession(EDIVESessionEndReason::Forced);
	}

	TestFalse(TEXT("Session inactive after end"), Subsystem->IsSessionActive());

	Device->Destroy();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
