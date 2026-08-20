// Copyright (c) 2026. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Actions/DIVEBuiltInActions.h"
#include "DIVEActionBinding.h"
#include "DIVEConvention.h"
#include "DIVEDeviceAction.h"
#include "DIVEDriveMapping.h"
#include "DIVEInspectableComponent.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVEPlayerComponent.h"
#include "DIVEProxyDrive.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVESessionSubsystem.h"
#include "DIVETypes.h"
#include "Utils/DIVEPlayerQuery.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/DefaultPawn.h"
#include "Tests/DIVEPawnPhysicalDriveTestTypes.h"
#include "Tests/DIVEProxyDriveTestTypes.h"
#include "UObject/Package.h"

#if WITH_EDITOR
#include "DIVEActionBindingValidation.h"
#include "Misc/DataValidation.h"
#include "UObject/UnrealType.h"
#endif

namespace
{
constexpr EAutomationTestFlags SmokeUnitTestFlags =
	EAutomationTestFlags::EditorContext
	| EAutomationTestFlags::ClientContext
	| EAutomationTestFlags::SmokeFilter;

ADIVETestPhysicalDrivePawn* SpawnSmokeHost(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	return World->SpawnActor<ADIVETestPhysicalDrivePawn>(
		ADIVETestPhysicalDrivePawn::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator);
}

USphereComponent* MakeSphereOnHost(AActor* Host, const FName Name, const float Radius)
{
	if (!Host)
	{
		return nullptr;
	}

	USphereComponent* Sphere = NewObject<USphereComponent>(Host, Name);
	if (!Sphere)
	{
		return nullptr;
	}

	Sphere->SetSphereRadius(Radius);
	Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	if (USceneComponent* Root = Host->GetRootComponent())
	{
		Sphere->SetupAttachment(Root);
	}
	else
	{
		Host->SetRootComponent(Sphere);
	}

	Host->AddInstanceComponent(Sphere);
	if (!Sphere->IsRegistered())
	{
		Sphere->RegisterComponent();
	}

	return Sphere;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEContextMenuDefaultBindingsSmokeTest,
	"DIVE.ContextMenu.DefaultBindings",
	SmokeUnitTestFlags)

bool FDIVEContextMenuDefaultBindingsSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

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
	TestFalse(
		TEXT("Focus CanExecute is false without a valid primitive pick"),
		Focus->CanExecute(FDIVEActionContext()));

	Binding.PrimaryActionIndex = 5;
	TestNull(TEXT("PrimaryAction null for invalid index"), Binding.GetPrimaryAction());

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
		FDIVEActionContext ContinuousContext;
		ContinuousContext.BindingId = TEXT("SmokeContinuous");
		Continuous->MarkInteractionActive(ContinuousContext);
		TestTrue(TEXT("MarkInteractionActive sets flag"), Continuous->IsInteractionActive());
		Continuous->NotifyValueChanged(1.5f);
		Continuous->NotifyValueChanged(-0.25f);
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

	const UDIVEInspectableComponent* InspectableCDO =
		UDIVEInspectableComponent::StaticClass()->GetDefaultObject<UDIVEInspectableComponent>();
	TestNotNull(TEXT("Inspectable CDO available"), InspectableCDO);
	if (!InspectableCDO)
	{
		return false;
	}

	FDIVETargetQuery TagQuery;
	TagQuery.MatchMode = EDIVETargetMatchMode::ComponentTag;
	TagQuery.MatchValues = { TEXT("DIVE.Bolt") };

	TArray<UPrimitiveComponent*> Matches;
	InspectableCDO->CollectPrimitivesMatchingQuery(TagQuery, Matches);
	TestEqual(TEXT("CDO Collect (no owner) is empty for Tag"), Matches.Num(), 0);

	FDIVETargetQuery AnyQuery;
	AnyQuery.MatchMode = EDIVETargetMatchMode::AnyPrimitive;
	InspectableCDO->CollectPrimitivesMatchingQuery(AnyQuery, Matches);
	TestEqual(TEXT("CDO Collect (no owner) is empty for AnyPrimitive"), Matches.Num(), 0);

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
		AddInfo(TEXT("No live Engine world — skipped owned CollectPrimitivesMatchingQuery assert."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Host = SpawnSmokeHost(World);
	TestNotNull(TEXT("Spawned collect-query host"), Host);
	if (!Host)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = NewObject<UDIVEInspectableComponent>(
		Host,
		UDIVEInspectableComponent::StaticClass(),
		TEXT("DIVEInspectable"));
	USphereComponent* Sphere = MakeSphereOnHost(Host, TEXT("BoltSphere"), 12.f);
	if (!Inspectable || !Sphere)
	{
		Host->Destroy();
		return false;
	}

	Host->AddInstanceComponent(Inspectable);
	if (!Inspectable->IsRegistered())
	{
		Inspectable->RegisterComponent();
	}
	Sphere->ComponentTags.Add(TEXT("DIVE.Bolt"));

	Inspectable->CollectPrimitivesMatchingQuery(TagQuery, Matches);
	TestTrue(TEXT("Owned Collect finds tagged sphere"), Matches.Contains(Sphere));

	Inspectable->CollectPrimitivesMatchingQuery(AnyQuery, Matches);
	TestTrue(TEXT("Owned Collect AnyPrimitive includes sphere"), Matches.Contains(Sphere));

	Host->Destroy();
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
			FDIVEActionWorldScope Outer(Action, World);
			TestEqual(TEXT("ExecutionWorld injected via scope"), Action->GetWorld(), World);
			{
				FDIVEActionWorldScope Inner(Action, World);
				TestEqual(TEXT("Nested WorldScope keeps injected world"), Action->GetWorld(), World);
			}
			TestEqual(TEXT("Nested WorldScope restores outer world"), Action->GetWorld(), World);
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
	FDIVEInspectableValidationSmokeTest,
	"DIVE.Inspectable.IsDataValid",
	SmokeUnitTestFlags)

bool FDIVEInspectableValidationSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

#if !WITH_EDITOR
	AddInfo(TEXT("IsDataValid / ValidateSections are editor-only — skipped."));
	return true;
#else
	{
		TArray<FDIVEMenuSection> Sections;
		FDIVEMenuSection Standard;
		Standard.SectionId = DIVE::kSectionStandard;
		FDIVEMenuSection Admin;
		Admin.SectionId = DIVE::kSectionAdmin;
		Sections.Add(Standard);
		Sections.Add(Admin);

		FDataValidationContext Context;
		TSet<FName> Known;
		TestTrue(
			TEXT("Authored Standard+Admin sections are valid"),
			DIVEActionBindingValidation::ValidateSections(Sections, Known, Context));
		TestTrue(TEXT("Known ids include Standard"), Known.Contains(DIVE::kSectionStandard));
		TestTrue(TEXT("Known ids include Admin"), Known.Contains(DIVE::kSectionAdmin));
	}

	{
		TArray<FDIVEMenuSection> DuplicateSections;
		FDIVEMenuSection First;
		First.SectionId = TEXT("Panel");
		FDIVEMenuSection Second;
		Second.SectionId = TEXT("Panel");
		DuplicateSections.Add(First);
		DuplicateSections.Add(Second);

		FDataValidationContext Context;
		TSet<FName> Known;
		TestFalse(
			TEXT("Duplicate SectionId fails ValidateSections"),
			DIVEActionBindingValidation::ValidateSections(DuplicateSections, Known, Context));
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
		AddInfo(TEXT("No live Engine world — skipped Inspectable IsDataValid on owned component."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Host = SpawnSmokeHost(World);
	TestNotNull(TEXT("Spawned inspectable-validation host"), Host);
	if (!Host)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = NewObject<UDIVEInspectableComponent>(
		Host,
		UDIVEInspectableComponent::StaticClass(),
		TEXT("DIVEInspectable"));
	TestNotNull(TEXT("Inspectable on validation host"), Inspectable);
	if (!Inspectable)
	{
		Host->Destroy();
		return false;
	}

	Host->AddInstanceComponent(Inspectable);
	if (!Inspectable->IsRegistered())
	{
		Inspectable->RegisterComponent();
	}

	FDataValidationContext OwnerContext;
	TestEqual(
		TEXT("Default Inspectable IsDataValid on live owner"),
		Inspectable->IsDataValid(OwnerContext),
		EDataValidationResult::Valid);

	Inspectable->bSeedAdminDefaults = false;
	if (FProperty* AdminSeedProp = FindFProperty<FProperty>(
			UDIVEInspectableComponent::StaticClass(),
			GET_MEMBER_NAME_CHECKED(UDIVEInspectableComponent, bSeedAdminDefaults)))
	{
		FPropertyChangedEvent ChangeEvent(AdminSeedProp);
		Inspectable->PostEditChangeProperty(ChangeEvent);
	}

	bool bHasSeededAdmin = false;
	for (const FDIVEActionBinding& Binding : Inspectable->Bindings)
	{
		if (Binding.BindingId == DIVE::kBindingBuiltInAdmin)
		{
			bHasSeededAdmin = true;
			break;
		}
	}
	TestFalse(TEXT("Unchecking Seed Admin Defaults removes BuiltIn.Admin"), bHasSeededAdmin);

	FDataValidationContext AfterAdminOff;
	TestEqual(
		TEXT("Inspectable still Valid after removing seeded Admin"),
		Inspectable->IsDataValid(AfterAdminOff),
		EDataValidationResult::Valid);

	Host->Destroy();
	return true;
#endif
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEDriveMappingSmokeTest,
	"DIVE.Drive.Mapping",
	SmokeUnitTestFlags)

bool FDIVEDriveMappingSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(
		TEXT("Zero delta maps to zero angle"),
		DIVE::MapScreenDeltaToAxisAngle(FVector2D::ZeroVector, FRotator::ZeroRotator, FVector::UpVector, 1.f),
		0.f);

	TestEqual(
		TEXT("Face-on axis falls back to horizontal pixels"),
		DIVE::MapScreenDeltaToAxisAngle(FVector2D(10.f, 0.f), FRotator::ZeroRotator, FVector::ForwardVector, 1.f),
		10.f);

	TestEqual(
		TEXT("Z-up drag right is clockwise from identity view"),
		DIVE::MapScreenDeltaToAxisAngle(FVector2D(10.f, 0.f), FRotator::ZeroRotator, FVector::UpVector, 1.f),
		-10.f);

	TestEqual(
		TEXT("Vertical drag projects onto world up as travel"),
		DIVE::MapScreenDeltaToAxisTravel(FVector2D(0.f, 10.f), FRotator::ZeroRotator, FVector::UpVector, 1.f),
		-10.f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEDriveBuiltInActionsSmokeTest,
	"DIVE.Drive.BuiltInActions",
	SmokeUnitTestFlags)

bool FDIVEDriveBuiltInActionsSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

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
		AddInfo(TEXT("No live Engine world — skipped rotary/threaded transform asserts."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Host = SpawnSmokeHost(World);
	TestNotNull(TEXT("Spawned drive-action host"), Host);
	if (!Host)
	{
		return false;
	}

	USphereComponent* Sphere = MakeSphereOnHost(Host, TEXT("DriveSphere"), 8.f);
	TestNotNull(TEXT("Drive sphere"), Sphere);
	if (!Sphere)
	{
		Host->Destroy();
		return false;
	}

	FDIVEActionContext Context;
	Context.Target = Sphere;
	Context.ViewRotation = FRotator::ZeroRotator;

	UDIVERotaryDriveAction* Rotary = NewObject<UDIVERotaryDriveAction>();
	TestNotNull(TEXT("Rotary action"), Rotary);
	if (Rotary)
	{
		Rotary->DegreesPerPixel = 1.f;
		Rotary->bLimitAngle = false;
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin"), Rotary->BeginInteraction(Context));
		Rotary->UpdateInteraction(FVector2D(10.f, 0.f), 0.016f);
		TestTrue(
			TEXT("Rotary accumulated yaw from Z-up mapping"),
			FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, -10.f, 0.05f));
		Rotary->EndInteraction(false);
		TestTrue(
			TEXT("Rotary cancel restores start rotation"),
			Sphere->GetRelativeRotation().IsNearlyZero(0.05f));

		Sphere->SetSimulatePhysics(true);
		TestFalse(TEXT("Rotary rejects simulating body"), Rotary->CanExecute(Context));
		Sphere->SetSimulatePhysics(false);
	}

	UDIVEThreadedDriveAction* Threaded = NewObject<UDIVEThreadedDriveAction>();
	TestNotNull(TEXT("Threaded action"), Threaded);
	if (Threaded)
	{
		Threaded->Axis = EDIVEDriveAxis::X;
		Threaded->DegreesPerPixel = 1.f;
		Threaded->TurnsToRelease = 1.f;
		Threaded->PitchCmPerTurn = 2.f;
		Threaded->MarkInteractionActive(Context);
		TestTrue(TEXT("Threaded Begin"), Threaded->BeginInteraction(Context));
		Threaded->UpdateInteraction(FVector2D(180.f, 0.f), 0.016f);
		TestTrue(
			TEXT("Threaded half-turn translates along local X"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 1.f, 0.05f));
		Threaded->EndInteraction(false);
		TestTrue(
			TEXT("Threaded cancel restores start transform"),
			Sphere->GetRelativeTransform().Equals(FTransform::Identity, 0.05f));

		Sphere->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		Threaded->MarkInteractionActive(Context);
		TestTrue(TEXT("Threaded Begin with pre-rotated part"), Threaded->BeginInteraction(Context));
		// Local X after yaw 90 is parent Y; identity view + vertical drag maps onto that axis.
		Threaded->UpdateInteraction(FVector2D(0.f, -180.f), 0.016f);
		TestTrue(
			TEXT("Threaded translation follows screw axis in parent space"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().Y, 1.f, 0.05f));
		TestTrue(
			TEXT("Threaded translation does not slide along parent X for a yaw-90 part"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 0.f, 0.05f));
		Threaded->EndInteraction(false);
		TestTrue(
			TEXT("Threaded cancel restores pre-rotated start transform"),
			Sphere->GetRelativeTransform().Equals(FTransform(FRotator(0.f, 90.f, 0.f)), 0.05f));
	}

	Host->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEProxyDriveResolveHierarchySmokeTest,
	"DIVE.ProxyDrive.ResolveHierarchy",
	SmokeUnitTestFlags)

bool FDIVEProxyDriveResolveHierarchySmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

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
		AddInfo(TEXT("No live Engine world — skipped proxy-drive hierarchy resolve."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Root = SpawnSmokeHost(World);
	ADIVETestPhysicalDrivePawn* Child = SpawnSmokeHost(World);
	TestNotNull(TEXT("Root host"), Root);
	TestNotNull(TEXT("Child host"), Child);
	if (!Root || !Child)
	{
		if (Root)
		{
			Root->Destroy();
		}
		if (Child)
		{
			Child->Destroy();
		}
		return false;
	}

	Child->AttachToActor(Root, FAttachmentTransformRules::KeepRelativeTransform);

	UDIVETestProxyDriveComponent* Drive = NewObject<UDIVETestProxyDriveComponent>(Root, TEXT("TestDrive"));
	UDIVETestControlRegistryComponent* Registry = NewObject<UDIVETestControlRegistryComponent>(Root, TEXT("TestRegistry"));
	TestNotNull(TEXT("Drive component"), Drive);
	TestNotNull(TEXT("Registry component"), Registry);
	if (!Drive || !Registry)
	{
		Root->Destroy();
		Child->Destroy();
		return false;
	}

	Registry->DriveObject = Drive;
	Root->AddInstanceComponent(Drive);
	Root->AddInstanceComponent(Registry);
	if (!Drive->IsRegistered())
	{
		Drive->RegisterComponent();
	}
	if (!Registry->IsRegistered())
	{
		Registry->RegisterComponent();
	}

	USphereComponent* ChildSphere = MakeSphereOnHost(Child, TEXT("ChildHit"), 8.f);
	TestNotNull(TEXT("Child hit sphere"), ChildSphere);
	if (!ChildSphere)
	{
		Root->Destroy();
		Child->Destroy();
		return false;
	}

	IDIVEProxyDrive* Resolved = DIVEProxyDriveResolve::FindProxyDriveForHit(ChildSphere);
	TestTrue(
		TEXT("Registry on root resolves for child-actor hit"),
		Resolved == Cast<IDIVEProxyDrive>(Drive));

	UDIVETestControlRegistryComponent* SecondRegistry =
		NewObject<UDIVETestControlRegistryComponent>(Root, TEXT("TestRegistry2"));
	UDIVETestProxyDriveComponent* Drive2 = NewObject<UDIVETestProxyDriveComponent>(Root, TEXT("TestDrive2"));
	if (SecondRegistry && Drive2)
	{
		SecondRegistry->DriveObject = Drive2;
		Root->AddInstanceComponent(Drive2);
		Root->AddInstanceComponent(SecondRegistry);
		if (!Drive2->IsRegistered())
		{
			Drive2->RegisterComponent();
		}
		if (!SecondRegistry->IsRegistered())
		{
			SecondRegistry->RegisterComponent();
		}
		TestNull(
			TEXT("N>1 registry results are fail-closed"),
			DIVEProxyDriveResolve::FindProxyDriveForHit(ChildSphere));
	}

	Child->Destroy();
	Root->Destroy();
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
