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
#include "Tests/DIVEActionValueSinkTestTypes.h"
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

FDIVEInteractionUpdate MakePolarUpdateAroundAxis(
	const FVector& AxisWorld,
	const float AngleDegrees,
	const float RimRadiusCm = 10.f,
	const float RayOffsetCm = 100.f)
{
	FDIVEInteractionUpdate Update;
	FVector BasisU = FVector::ZeroVector;
	FVector BasisV = FVector::ZeroVector;
	DIVE::BuildAxisPlaneBasis(AxisWorld, BasisU, BasisV);

	const FVector Axis = AxisWorld.GetSafeNormal();
	const float Rad = FMath::DegreesToRadians(AngleDegrees);
	const FVector RimPoint = BasisU * (RimRadiusCm * FMath::Cos(Rad)) + BasisV * (RimRadiusCm * FMath::Sin(Rad));
	// Offset along the axis so the ray is not parallel to the drive plane.
	const FVector RayStart = RimPoint + Axis * RayOffsetCm;
	Update.ViewLocation = RayStart;
	Update.PickRayDir = -Axis;
	Update.DeltaTime = 0.016f;
	return Update;
}

FDIVEInteractionUpdate MakeZUpPolarUpdate(const float AngleDegrees, const float RimRadiusCm = 10.f)
{
	return MakePolarUpdateAroundAxis(FVector::UpVector, AngleDegrees, RimRadiusCm);
}

FDIVEInteractionUpdate MakeXAxisPolarUpdate(const float AngleDegrees, const float RimRadiusCm = 10.f)
{
	return MakePolarUpdateAroundAxis(FVector::ForwardVector, AngleDegrees, RimRadiusCm);
}

/** Ray perpendicular to AxisWorld whose closest point on the axis sits at ParameterCm from the origin. */
FDIVEInteractionUpdate MakeLinearUpdateAlongAxis(
	const FVector& AxisWorld,
	const float ParameterCm,
	const float LateralOffsetCm = 100.f)
{
	FDIVEInteractionUpdate Update;
	FVector BasisU = FVector::ZeroVector;
	FVector BasisV = FVector::ZeroVector;
	DIVE::BuildAxisPlaneBasis(AxisWorld, BasisU, BasisV);

	const FVector Axis = AxisWorld.GetSafeNormal();
	const FVector PointOnAxis = Axis * ParameterCm;
	const FVector RayStart = PointOnAxis + BasisU * LateralOffsetCm;
	Update.ViewLocation = RayStart;
	Update.PickRayDir = -BasisU;
	Update.DeltaTime = 0.016f;
	return Update;
}

FDIVEInteractionUpdate MakeXAxisLinearUpdate(const float ParameterCm)
{
	return MakeLinearUpdateAlongAxis(FVector::ForwardVector, ParameterCm);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEContextMenuDefaultBindingsSmokeTest,
	"DIVE.ContextMenu.DefaultBindings",
	SmokeUnitTestFlags)

bool FDIVEContextMenuDefaultBindingsSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// Native CDO must stay empty: seeding named NewObject actions on it makes device BPs
	// save-illegal (private Default__DIVEInspectableComponent:DefaultDeleteMeshAction refs).
	const UDIVEInspectableComponent* InspectableCdo =
		UDIVEInspectableComponent::StaticClass()->GetDefaultObject<UDIVEInspectableComponent>();
	TestNotNull(TEXT("Inspectable CDO available"), InspectableCdo);
	if (!InspectableCdo)
	{
		return false;
	}
	TestTrue(TEXT("Native Inspectable CDO has no Bindings"), InspectableCdo->Bindings.IsEmpty());
	TestFalse(TEXT("CDO has no hover overlay authored"), InspectableCdo->HasPickHoverOverlay());
	UDIVEInspectableComponent* MutableCdo =
		const_cast<UDIVEInspectableComponent*>(InspectableCdo);
	TestNull(
		TEXT("CDO has no DefaultFocusAction inner"),
		FindObject<UDIVEFocusAction>(MutableCdo, DIVE::kSeededFocusAction));
	TestNull(
		TEXT("CDO has no DefaultIsolateAction inner"),
		FindObject<UDIVEIsolateAction>(MutableCdo, DIVE::kSeededIsolateAction));
	TestNull(
		TEXT("CDO has no DefaultSimulatePhysicsAction inner"),
		FindObject<UDIVESimulatePhysicsAction>(MutableCdo, DIVE::kSeededSimulatePhysicsAction));
	TestNull(
		TEXT("CDO has no DefaultDeleteMeshAction inner"),
		FindObject<UDIVEDeleteMeshAction>(MutableCdo, DIVE::kSeededDeleteMeshAction));

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
		AddInfo(TEXT("No live Engine world — skipped instance default-binding asserts."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Host = SpawnSmokeHost(World);
	TestNotNull(TEXT("Spawned default-bindings host"), Host);
	if (!Host)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = NewObject<UDIVEInspectableComponent>(
		Host,
		UDIVEInspectableComponent::StaticClass(),
		TEXT("DIVEInspectable"));
	TestNotNull(TEXT("Inspectable instance"), Inspectable);
	if (!Inspectable)
	{
		Host->Destroy();
		return false;
	}

	Host->AddInstanceComponent(Inspectable);

	TestTrue(TEXT("New Inspectable seeds Bindings"), Inspectable->Bindings.Num() >= 2);

	bool bSeededFocusOwned = false;
	bool bSeededInnersPublic = true;
	int32 SeededActionCount = 0;
	for (const FDIVEActionBinding& Binding : Inspectable->Bindings)
	{
		for (UDIVEDeviceAction* Action : Binding.Actions)
		{
			if (!Action)
			{
				continue;
			}

			++SeededActionCount;
			bSeededInnersPublic &= Action->HasAnyFlags(RF_Public);
			if (Action->IsA<UDIVEFocusAction>())
			{
				bSeededFocusOwned = Action->GetOuter() == Inspectable;
			}
		}
	}
	TestTrue(TEXT("Seeded Focus is owned by the Inspectable"), bSeededFocusOwned);
	TestTrue(TEXT("Seeded actions are RF_Public"), bSeededInnersPublic);
	TestTrue(TEXT("New Inspectable seeds Standard+Admin actions"), SeededActionCount >= 4);

	TArray<const FDIVEActionBinding*> GatheredBindings;
	Inspectable->GatherAuthoredBindings(GatheredBindings);

	bool bFoundFocus = false;
	bool bAnyPrimitiveBinding = false;
	bool bFoundAdmin = false;
	bool bFoundSimulate = false;
	bool bFoundDelete = false;
	const FDIVEActionBinding* StandardBinding = nullptr;
	for (const FDIVEActionBinding* Binding : GatheredBindings)
	{
		if (!Binding)
		{
			continue;
		}
		if (Binding->Targets.MatchMode == EDIVETargetMatchMode::AnyPrimitive)
		{
			bAnyPrimitiveBinding = true;
		}
		if (Binding->BindingId == DIVE::kBindingBuiltInStandard)
		{
			StandardBinding = Binding;
		}
		if (Binding->BindingId == DIVE::kBindingBuiltInAdmin)
		{
			bFoundAdmin = true;
		}
		for (UDIVEDeviceAction* Action : Binding->Actions)
		{
			bFoundFocus |= Action && Action->IsA<UDIVEFocusAction>();
			bFoundSimulate |= Action && Action->IsA<UDIVESimulatePhysicsAction>();
			bFoundDelete |= Action && Action->IsA<UDIVEDeleteMeshAction>();
		}
	}
	TestTrue(TEXT("Built-in Bindings include Focus"), bFoundFocus);
	TestTrue(TEXT("Built-in Bindings use AnyPrimitive"), bAnyPrimitiveBinding);
	TestNotNull(TEXT("Built-in Standard binding present"), StandardBinding);
	TestTrue(TEXT("Built-in Admin binding present"), bFoundAdmin);
	TestTrue(TEXT("Built-in Admin includes Simulate Physics"), bFoundSimulate);
	TestTrue(TEXT("Built-in Admin includes Delete Mesh"), bFoundDelete);

	FDIVEFocusTarget DummyPick = FDIVEFocusTarget::FromPrimitive(nullptr, NAME_None);
	TestFalse(TEXT("AnyPrimitive does not match invalid primitive pick"),
		StandardBinding && Inspectable->DoesTargetQueryMatchPick(StandardBinding->Targets, DummyPick));

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

	UDIVEDeleteMeshAction* ForeignDelete = NewObject<UDIVEDeleteMeshAction>(
		GetTransientPackage(),
		UDIVEDeleteMeshAction::StaticClass(),
		TEXT("ForeignDeleteMesh"));
	TestNotNull(TEXT("Foreign DeleteMesh action"), ForeignDelete);
	if (ForeignDelete)
	{
		FDIVEActionBinding ForeignBinding;
		ForeignBinding.BindingId = TEXT("Test.Foreign");
		ForeignBinding.Actions.Add(ForeignDelete);
		Inspectable->Bindings.Add(ForeignBinding);
	}

	if (Inspectable->IsRegistered())
	{
		Inspectable->UnregisterComponent();
	}
	Inspectable->RegisterComponent();

	const FDIVEActionBinding* ForeignSlot = nullptr;
	for (const FDIVEActionBinding& Binding : Inspectable->Bindings)
	{
		if (Binding.BindingId == TEXT("Test.Foreign"))
		{
			ForeignSlot = &Binding;
			break;
		}
	}
	TestNotNull(TEXT("Inspectable Bindings has the foreign slot"), ForeignSlot);
	if (ForeignSlot && ForeignSlot->Actions.Num() == 1)
	{
		UDIVEDeviceAction* InstancedDelete = ForeignSlot->Actions[0];
		TestNotNull(TEXT("Instanced DeleteMesh after register"), InstancedDelete);
		TestEqual(
			TEXT("Foreign private action is instanced onto the Inspectable"),
			InstancedDelete ? InstancedDelete->GetOuter() : nullptr,
			static_cast<UObject*>(Inspectable));
		TestTrue(
			TEXT("Instanced DeleteMesh is not the original foreign object"),
			InstancedDelete != ForeignDelete);
	}

	GatheredBindings.Reset();
	Inspectable->GatherAuthoredBindings(GatheredBindings);
	bool bFocusAfterCustomBinding = false;
	for (const FDIVEActionBinding* Binding : GatheredBindings)
	{
		if (!Binding)
		{
			continue;
		}
		for (UDIVEDeviceAction* Action : Binding->Actions)
		{
			if (Action && Action->IsA<UDIVEFocusAction>())
			{
				bFocusAfterCustomBinding = true;
			}
		}
	}
	TestTrue(TEXT("Built-in Focus remains after a custom Binding"), bFocusAfterCustomBinding);

	Host->Destroy();
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

	{
		UDIVERotaryDriveAction* SoleRotary = NewObject<UDIVERotaryDriveAction>();
		TestNotNull(TEXT("Sole rotary for implicit primary"), SoleRotary);
		if (SoleRotary)
		{
			FDIVEActionBinding ImplicitRotary;
			ImplicitRotary.Actions.Add(SoleRotary);
			TestEqual(
				TEXT("Sole continuous action is implicit LMB primary"),
				ImplicitRotary.GetPrimaryAction(),
				static_cast<UDIVEDeviceAction*>(SoleRotary));
		}

		UDIVEMomentaryPressAction* SolePress = NewObject<UDIVEMomentaryPressAction>();
		TestNotNull(TEXT("Sole momentary press for implicit primary"), SolePress);
		if (SolePress)
		{
			FDIVEActionBinding ImplicitPress;
			ImplicitPress.Actions.Add(SolePress);
			TestEqual(
				TEXT("Sole Momentary Press is implicit LMB primary"),
				ImplicitPress.GetPrimaryAction(),
				static_cast<UDIVEDeviceAction*>(SolePress));
		}

		FDIVEActionBinding SoleFocus;
		SoleFocus.Actions.Add(Focus);
		TestNull(
			TEXT("Sole instant action is not implicit LMB primary"),
			SoleFocus.GetPrimaryAction());
	}

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
	FDIVEActionsHeadlessExecuteSmokeTest,
	"DIVE.Actions.HeadlessExecute",
	SmokeUnitTestFlags)

bool FDIVEActionsHeadlessExecuteSmokeTest::RunTest(const FString& Parameters)
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
		AddInfo(TEXT("No live Engine world — skipped headless execute."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Device = SpawnSmokeHost(World);
	TestNotNull(TEXT("Headless host"), Device);
	if (!Device)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = NewObject<UDIVEInspectableComponent>(
		Device,
		UDIVEInspectableComponent::StaticClass(),
		TEXT("DIVEInspectable"));
	TestNotNull(TEXT("Inspectable"), Inspectable);
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

	USphereComponent* HitSphere = MakeSphereOnHost(Device, TEXT("HitPrim"), 12.f);
	TestNotNull(TEXT("Hit sphere"), HitSphere);
	if (!HitSphere)
	{
		Device->Destroy();
		return false;
	}

	UDIVENotifyAction* Notify = NewObject<UDIVENotifyAction>(Inspectable, TEXT("HeadlessNotify"));
	TestNotNull(TEXT("Notify action"), Notify);
	if (!Notify)
	{
		Device->Destroy();
		return false;
	}

	FDIVEActionBinding Binding;
	Binding.BindingId = TEXT("HeadlessNotifyBinding");
	Binding.Targets.MatchMode = EDIVETargetMatchMode::AnyPrimitive;
	Binding.SectionId = DIVE::kSectionStandard;
	Binding.PrimaryActionIndex = 0;
	Binding.Actions.Add(Notify);
	Inspectable->Bindings.Add(Binding);

	const FDIVEFocusTarget Pick = FDIVEFocusTarget::FromPrimitive(HitSphere, NAME_None);
	const FDIVEActionContext Context = Inspectable->MakeActionContext(
		Pick,
		NAME_None,
		FVector2D(10.f, 10.f),
		FHitResult(),
		Binding.BindingId,
		true,
		FVector(0.f, 0.f, 100.f),
		FRotator(-90.f, 0.f, 0.f));

	TestEqual(TEXT("View override applied"), Context.ViewLocation.Z, 100.0);
	TestFalse(TEXT("No camera session required"), Inspectable->IsSessionActive());

	TestTrue(TEXT("Headless Notify ExecuteAction"), Inspectable->ExecuteAction(Notify, Context));
	TestFalse(TEXT("Notify leaves no continuous slot"), Inspectable->HasActiveInteraction());

	// Continuous Begin/Update/End without a camera session (kinematic rotary on the hit sphere).
	UDIVERotaryDriveAction* Rotary = NewObject<UDIVERotaryDriveAction>(Inspectable, TEXT("HeadlessRotary"));
	TestNotNull(TEXT("Rotary action"), Rotary);
	if (Rotary)
	{
		Rotary->bLimitAngle = false;
		const FDIVEActionContext RotaryContext = Inspectable->MakeActionContext(
			Pick,
			NAME_None,
			FVector2D::ZeroVector,
			FHitResult(),
			NAME_None,
			true,
			FVector(0.f, 0.f, 200.f),
			FRotator(-90.f, 0.f, 0.f));

		TestTrue(TEXT("Headless Rotary Begin"), Inspectable->ExecuteAction(Rotary, RotaryContext));
		TestTrue(TEXT("Headless continuous active after Begin"), Inspectable->HasActiveInteraction());
		if (Inspectable->HasActiveInteraction())
		{
			FDIVEInteractionUpdate Update;
			Update.DeltaTime = 0.016f;
			Update.ScreenDelta = FVector2D(4.f, 0.f);
			Update.ScreenPosition = FVector2D(100.f, 100.f);
			Update.ViewLocation = RotaryContext.ViewLocation;
			Update.ViewRotation = RotaryContext.ViewRotation;
			Update.PickRayDir = RotaryContext.PickRayDir;
			Inspectable->UpdateActiveInteraction(Update);
			Inspectable->EndActiveInteraction(true);
			TestFalse(TEXT("Headless continuous cleared after End"), Inspectable->HasActiveInteraction());
		}
	}

	// Presentation filter: Focus is Session-only.
	UDIVEFocusAction* Focus = NewObject<UDIVEFocusAction>(Inspectable, TEXT("HeadlessFocus"));
	TestNotNull(TEXT("Focus action"), Focus);
	if (Focus)
	{
		TestEqual(
			TEXT("Focus Presentation is Session"),
			Focus->Presentation,
			EDIVEActionPresentation::Session);
		TestTrue(
			TEXT("Focus matches Session filter"),
			ActionMatchesPresentationFilter(Focus->Presentation, EDIVEActionPresentation::Session));
		TestFalse(
			TEXT("Focus hidden from World filter"),
			ActionMatchesPresentationFilter(Focus->Presentation, EDIVEActionPresentation::World));
	}

	TArray<FDIVEContextMenuEntry> WorldEntries;
	Inspectable->AppendConfiguredContextMenuEntries(Pick, WorldEntries, EDIVEActionPresentation::World);
	bool bWorldHasFocus = false;
	bool bWorldHasNotify = false;
	for (const FDIVEContextMenuEntry& Entry : WorldEntries)
	{
		if (Entry.Action && Entry.Action->IsA<UDIVEFocusAction>())
		{
			bWorldHasFocus = true;
		}
		if (Entry.Action && Entry.Action->IsA<UDIVENotifyAction>())
		{
			bWorldHasNotify = true;
		}
	}
	TestFalse(TEXT("World menu omits Focus"), bWorldHasFocus);
	TestTrue(TEXT("World menu keeps Notify (Both)"), bWorldHasNotify);

	TArray<FDIVEContextMenuEntry> SessionEntries;
	Inspectable->AppendConfiguredContextMenuEntries(Pick, SessionEntries, EDIVEActionPresentation::Session);
	bool bSessionHasFocus = false;
	for (const FDIVEContextMenuEntry& Entry : SessionEntries)
	{
		if (Entry.Action && Entry.Action->IsA<UDIVEFocusAction>())
		{
			bSessionHasFocus = true;
			break;
		}
	}
	TestTrue(TEXT("Session menu includes seeded Focus"), bSessionHasFocus);

	// Focus Execute via headless must not start a camera session.
	if (Focus)
	{
		(void)Inspectable->ExecuteAction(Focus, Context);
		TestFalse(
			TEXT("Headless Focus Execute does not activate a camera session"),
			Inspectable->IsSessionActive());
	}

	Device->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEActionsMomentaryPressSmokeTest,
	"DIVE.Actions.MomentaryPress",
	SmokeUnitTestFlags)

bool FDIVEActionsMomentaryPressSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const UClass* PressClass = UDIVEMomentaryPressAction::StaticClass();
	TestNotNull(TEXT("MomentaryPress class"), PressClass);
	if (PressClass)
	{
		TestFalse(
			TEXT("MomentaryPress is authorable in Catalog (not HideDropdown)"),
			PressClass->HasAnyClassFlags(CLASS_HideDropDown));
		TestTrue(
			TEXT("MomentaryPress is a continuous action"),
			PressClass->IsChildOf(UDIVEContinuousDeviceAction::StaticClass()));
	}

	UDIVEMomentaryPressAction* Press = NewObject<UDIVEMomentaryPressAction>();
	TestNotNull(TEXT("MomentaryPress instance"), Press);
	if (!Press)
	{
		return false;
	}

	const FDIVEActionContext EmptyContext;
	TestTrue(TEXT("MomentaryPress CanExecute without a pick"), Press->CanExecute(EmptyContext));
	TestFalse(TEXT("MomentaryPress Execute is not instant"), Press->Execute(EmptyContext));

	UDIVETestActionValueSink* Sink = NewObject<UDIVETestActionValueSink>();
	TestNotNull(TEXT("Value sink"), Sink);
	if (!Sink)
	{
		return false;
	}

	Press->OnValueChanged.AddUniqueDynamic(Sink, &UDIVETestActionValueSink::HandleValue);
	Press->MarkInteractionActive(EmptyContext);
	TestTrue(TEXT("MomentaryPress Begin"), Press->BeginInteraction(EmptyContext));
	TestTrue(TEXT("Pressed while the slot is live"), Press->IsInteractionActive());
	TestEqual(TEXT("Begin emits pressed (1)"), Sink->Normalized.Num(), 1);
	if (Sink->Normalized.Num() >= 1)
	{
		TestEqual(TEXT("Pressed normalized"), Sink->Normalized[0], 1.f);
	}

	FDIVEInteractionUpdate Drag;
	Drag.ScreenDelta = FVector2D(12.f, -8.f);
	Drag.DeltaTime = 0.016f;
	Press->UpdateInteraction(Drag);
	TestEqual(TEXT("Update does not re-emit (edges only)"), Sink->Normalized.Num(), 1);

	Press->EndInteraction(true);
	TestFalse(TEXT("Released after End"), Press->IsInteractionActive());
	TestEqual(TEXT("End emits released (0)"), Sink->Normalized.Num(), 2);
	if (Sink->Normalized.Num() >= 2)
	{
		TestEqual(TEXT("Released normalized"), Sink->Normalized[1], 0.f);
	}

	Press->EndInteraction(false);
	TestEqual(TEXT("Second End does not emit another 0"), Sink->Normalized.Num(), 2);

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
		AddInfo(TEXT("No live Engine world — skipped headless MomentaryPress execute."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Device = SpawnSmokeHost(World);
	TestNotNull(TEXT("Headless host"), Device);
	if (!Device)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = NewObject<UDIVEInspectableComponent>(
		Device,
		UDIVEInspectableComponent::StaticClass(),
		TEXT("DIVEInspectable"));
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

	USphereComponent* HitSphere = MakeSphereOnHost(Device, TEXT("PressPrim"), 12.f);
	UDIVEMomentaryPressAction* HeadlessPress = NewObject<UDIVEMomentaryPressAction>(
		Inspectable,
		TEXT("HeadlessPress"));
	if (!HitSphere || !HeadlessPress)
	{
		Device->Destroy();
		return false;
	}

	const FDIVEFocusTarget Pick = FDIVEFocusTarget::FromPrimitive(HitSphere, NAME_None);
	const FDIVEActionContext Context = Inspectable->MakeActionContext(
		Pick,
		NAME_None,
		FVector2D(10.f, 10.f),
		FHitResult(),
		NAME_None,
		true,
		FVector(0.f, 0.f, 100.f),
		FRotator(-90.f, 0.f, 0.f));

	UDIVETestActionValueSink* HeadlessSink = NewObject<UDIVETestActionValueSink>();
	if (!HeadlessSink)
	{
		Device->Destroy();
		return false;
	}

	Inspectable->OnActionValueChanged.AddUniqueDynamic(
		HeadlessSink,
		&UDIVETestActionValueSink::HandleValue);

	TestTrue(TEXT("Headless MomentaryPress Begin"), Inspectable->ExecuteAction(HeadlessPress, Context));
	TestTrue(TEXT("Headless press occupies the Inspectable slot"), Inspectable->HasActiveInteraction());
	Inspectable->EndActiveInteraction(false);
	TestFalse(TEXT("Headless press cleared after End"), Inspectable->HasActiveInteraction());
	TestEqual(TEXT("Headless executor emits 1 then 0"), HeadlessSink->Normalized.Num(), 2);
	if (HeadlessSink->Normalized.Num() >= 2)
	{
		TestEqual(TEXT("Headless pressed"), HeadlessSink->Normalized[0], 1.f);
		TestEqual(TEXT("Headless released (cancel still 0)"), HeadlessSink->Normalized[1], 0.f);
	}

	Device->Destroy();
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

	auto HasBuiltInAdmin = [](UDIVEInspectableComponent* Component) -> bool
	{
		TArray<const FDIVEActionBinding*> Gathered;
		Component->GatherAuthoredBindings(Gathered);
		for (const FDIVEActionBinding* Binding : Gathered)
		{
			if (Binding && Binding->BindingId == DIVE::kBindingBuiltInAdmin)
			{
				return true;
			}
		}
		return false;
	};

	TestTrue(TEXT("New Inspectable seeds BuiltIn.Admin"), HasBuiltInAdmin(Inspectable));

	Inspectable->Bindings.RemoveAll([](const FDIVEActionBinding& Binding)
	{
		return Binding.BindingId == DIVE::kBindingBuiltInAdmin;
	});
	TestFalse(TEXT("Removing BuiltIn.Admin leaves it gone"), HasBuiltInAdmin(Inspectable));

	Inspectable->AddAdminDefaultBindings();
	TestTrue(TEXT("Add Admin Defaults restores BuiltIn.Admin"), HasBuiltInAdmin(Inspectable));

	for (FDIVEActionBinding& Binding : Inspectable->Bindings)
	{
		if (Binding.BindingId == DIVE::kBindingBuiltInAdmin)
		{
			Binding.Actions.Reset();
			break;
		}
	}
	Inspectable->AddAdminDefaultBindings();

	bool bRestoredSimulate = false;
	bool bRestoredDelete = false;
	for (const FDIVEActionBinding& Binding : Inspectable->Bindings)
	{
		if (Binding.BindingId != DIVE::kBindingBuiltInAdmin)
		{
			continue;
		}

		for (UDIVEDeviceAction* Action : Binding.Actions)
		{
			bRestoredSimulate |= Action && Action->IsA<UDIVESimulatePhysicsAction>();
			bRestoredDelete |= Action && Action->IsA<UDIVEDeleteMeshAction>();
		}
	}
	TestTrue(TEXT("Add Admin Defaults restores Simulate Physics"), bRestoredSimulate);
	TestTrue(TEXT("Add Admin Defaults restores Delete Mesh"), bRestoredDelete);

	auto CountAdminBindings = [](const UDIVEInspectableComponent* Component) -> int32
	{
		int32 Count = 0;
		for (const FDIVEActionBinding& Binding : Component->Bindings)
		{
			if (Binding.BindingId == DIVE::kBindingBuiltInAdmin)
			{
				++Count;
			}
		}
		return Count;
	};
	TestEqual(TEXT("Add Admin Defaults leaves a single Admin binding"), CountAdminBindings(Inspectable), 1);
	Inspectable->AddAdminDefaultBindings();
	TestEqual(TEXT("Add Admin Defaults is idempotent"), CountAdminBindings(Inspectable), 1);

	FDataValidationContext AfterAdminRestore;
	TestEqual(
		TEXT("Inspectable still Valid after Add Admin Defaults"),
		Inspectable->IsDataValid(AfterAdminRestore),
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

	TestEqual(
		TEXT("Looking along the rail falls back to horizontal pixels"),
		DIVE::MapScreenDeltaToAxisTravel(FVector2D(10.f, 0.f), FRotator::ZeroRotator, FVector::ForwardVector, 1.f),
		10.f);

	{
		const FVector AxisOrigin = FVector::ZeroVector;
		const FVector AxisWorld = FVector::UpVector;
		FVector BasisU = FVector::ZeroVector;
		FVector BasisV = FVector::ZeroVector;
		DIVE::BuildAxisPlaneBasis(AxisWorld, BasisU, BasisV);
		const float RimRadiusCm = 10.f;
		const float DeadRadiusCm = 1.f;

		const auto RayAtAngle = [&](const float AngleDegrees) -> TPair<FVector, FVector>
		{
			const FDIVEInteractionUpdate Frame = MakeZUpPolarUpdate(AngleDegrees, RimRadiusCm);
			return TPair<FVector, FVector>(Frame.ViewLocation, Frame.PickRayDir);
		};

		const FVector AngleZeroVector = BasisU * RimRadiusCm;

		{
			const TPair<FVector, FVector> Ray = RayAtAngle(0.f);
			const FDIVEPointerAxisAngleResult Seed = DIVE::MapPointerToAxisAngle(
				AxisOrigin,
				AxisWorld,
				BasisU,
				BasisV,
				Ray.Key,
				Ray.Value,
				FVector::ZeroVector,
				DeadRadiusCm);
			TestFalse(TEXT("Polar seed with no previous vector does not apply"), Seed.bApplied);
			TestTrue(
				TEXT("Polar seed still exposes the rim vector"),
				Seed.PlaneVector.Size() >= DeadRadiusCm);
		}

		FVector PreviousPlaneVector = AngleZeroVector;
		{
			const TPair<FVector, FVector> Ray = RayAtAngle(0.f);
			const FDIVEPointerAxisAngleResult SameAngle = DIVE::MapPointerToAxisAngle(
				AxisOrigin,
				AxisWorld,
				BasisU,
				BasisV,
				Ray.Key,
				Ray.Value,
				PreviousPlaneVector,
				DeadRadiusCm);
			TestTrue(TEXT("Polar repeat angle applies zero delta"), SameAngle.bApplied);
			TestTrue(
				TEXT("Polar repeat angle delta is zero"),
				FMath::IsNearlyZero(SameAngle.DeltaDegrees, 0.05f));
			PreviousPlaneVector = SameAngle.PlaneVector;
		}

		float WrappedTotal = 0.f;
		PreviousPlaneVector = AngleZeroVector;
		for (int32 Step = 1; Step <= 36; ++Step)
		{
			const TPair<FVector, FVector> Ray = RayAtAngle(Step * 10.f);
			const FDIVEPointerAxisAngleResult StepResult = DIVE::MapPointerToAxisAngle(
				AxisOrigin,
				AxisWorld,
				BasisU,
				BasisV,
				Ray.Key,
				Ray.Value,
				PreviousPlaneVector,
				DeadRadiusCm);
			if (StepResult.bApplied)
			{
				WrappedTotal += StepResult.DeltaDegrees;
				PreviousPlaneVector = StepResult.PlaneVector;
			}
		}
		TestTrue(
			TEXT("Polar full revolution wraps to about 360 degrees"),
			FMath::IsNearlyEqual(WrappedTotal, 360.f, 0.5f));

		{
			const FDIVEPointerAxisAngleResult DeadZone = DIVE::MapPointerToAxisAngle(
				AxisOrigin,
				AxisWorld,
				BasisU,
				BasisV,
				FVector(0.5f, 0.f, 100.f),
				FVector(0.f, 0.f, -1.f),
				AngleZeroVector,
				DeadRadiusCm);
			TestFalse(TEXT("Polar dead zone rejects the hit"), DeadZone.bApplied);
			TestTrue(
				TEXT("Polar dead zone keeps zero delta"),
				FMath::IsNearlyZero(DeadZone.DeltaDegrees, 0.05f));
		}

		{
			const FDIVEPointerAxisAngleResult FromPoint = DIVE::MapWorldPointToAxisAngle(
				AxisOrigin,
				AxisWorld,
				BasisU,
				BasisV,
				AngleZeroVector,
				AngleZeroVector,
				DeadRadiusCm);
			TestTrue(TEXT("World-point polar at the same rim point applies"), FromPoint.bApplied);
			TestTrue(
				TEXT("World-point polar same-point delta is zero"),
				FMath::IsNearlyZero(FromPoint.DeltaDegrees, 0.05f));
		}

		{
			const FDIVEPointerAxisAngleResult Grazing = DIVE::MapPointerToAxisAngle(
				AxisOrigin,
				AxisWorld,
				BasisU,
				BasisV,
				FVector::ZeroVector,
				FVector::ForwardVector,
				AngleZeroVector,
				DeadRadiusCm);
			TestFalse(TEXT("Polar grazing ray rejects the hit"), Grazing.bApplied);
		}
	}

	{
		const FVector AxisOrigin = FVector::ZeroVector;
		const FVector AxisWorld = FVector::ForwardVector;

		{
			const FDIVEPointerAxisTravelResult Seed = DIVE::MapWorldPointToAxisTravel(
				AxisOrigin,
				AxisWorld,
				FVector(5.f, 0.f, 0.f),
				false,
				0.f);
			TestFalse(TEXT("Travel seed with no previous does not apply"), Seed.bApplied);
			TestTrue(TEXT("Travel seed still exposes the parameter"), Seed.bHasParameter);
			TestTrue(
				TEXT("Travel seed parameter is 5 cm"),
				FMath::IsNearlyEqual(Seed.ParameterCm, 5.f, 0.05f));
		}

		{
			const FDIVEPointerAxisTravelResult SamePoint = DIVE::MapWorldPointToAxisTravel(
				AxisOrigin,
				AxisWorld,
				FVector(5.f, 0.f, 0.f),
				true,
				5.f);
			TestTrue(TEXT("Travel same point applies"), SamePoint.bApplied);
			TestTrue(
				TEXT("Travel same-point delta is zero"),
				FMath::IsNearlyZero(SamePoint.DeltaCm, 0.05f));
		}

		{
			const FDIVEPointerAxisTravelResult Shifted = DIVE::MapWorldPointToAxisTravel(
				AxisOrigin,
				AxisWorld,
				FVector(5.f, 2.f, -1.f),
				true,
				0.f);
			TestTrue(TEXT("Travel shift along axis applies"), Shifted.bApplied);
			TestTrue(
				TEXT("Travel shift delta is about 5 cm"),
				FMath::IsNearlyEqual(Shifted.DeltaCm, 5.f, 0.05f));
		}

		{
			const FDIVEInteractionUpdate Frame = MakeXAxisLinearUpdate(0.f);
			const FDIVEPointerAxisTravelResult SeedRay = DIVE::MapPointerToAxisTravel(
				AxisOrigin,
				AxisWorld,
				Frame.ViewLocation,
				Frame.PickRayDir,
				false,
				0.f);
			TestFalse(TEXT("Pointer travel seed does not apply"), SeedRay.bApplied);
			TestTrue(TEXT("Pointer travel seed has a parameter"), SeedRay.bHasParameter);
			TestTrue(
				TEXT("Pointer travel seed parameter is about 0"),
				FMath::IsNearlyZero(SeedRay.ParameterCm, 0.05f));
		}

		{
			const FDIVEInteractionUpdate AtFive = MakeXAxisLinearUpdate(5.f);
			const FDIVEPointerAxisTravelResult Moved = DIVE::MapPointerToAxisTravel(
				AxisOrigin,
				AxisWorld,
				AtFive.ViewLocation,
				AtFive.PickRayDir,
				true,
				0.f);
			TestTrue(TEXT("Pointer travel along X applies"), Moved.bApplied);
			TestTrue(
				TEXT("Pointer travel delta is about 5 cm"),
				FMath::IsNearlyEqual(Moved.DeltaCm, 5.f, 0.05f));
		}

		{
			const FDIVEPointerAxisTravelResult Parallel = DIVE::MapPointerToAxisTravel(
				AxisOrigin,
				AxisWorld,
				FVector::ZeroVector,
				FVector::ForwardVector,
				true,
				0.f);
			TestFalse(TEXT("Pointer travel rejects a ray parallel to the axis"), Parallel.bApplied);
			TestFalse(TEXT("Parallel travel ray has no parameter"), Parallel.bHasParameter);
		}
	}

	{
		FDIVEInteractionValue NoneValue;
		NoneValue.Normalized = 0.42f;
		const FString NoneText =
			DIVE::FormatInteractionValueReadout(FText::FromString(TEXT("Proxy")), NoneValue).ToString();
		TestTrue(TEXT("None unit readout includes the action label"), NoneText.Contains(TEXT("Proxy")));

		FDIVEInteractionValue DegValue;
		DegValue.Unit = EDIVEInteractionValueUnit::Degrees;
		DegValue.Absolute = -10.f;
		const FString DegText =
			DIVE::FormatInteractionValueReadout(FText::FromString(TEXT("Knob")), DegValue).ToString();
		TestTrue(TEXT("Degrees readout includes the action label"), DegText.Contains(TEXT("Knob")));
		TestTrue(TEXT("Degrees readout includes the degree glyph"), DegText.Contains(TEXT("°")));

		FDIVEInteractionValue TurnsValue;
		TurnsValue.Unit = EDIVEInteractionValueUnit::Turns;
		TurnsValue.Absolute = 2.5f;
		TurnsValue.AbsoluteMax = 4.f;
		const FString TurnsText =
			DIVE::FormatInteractionValueReadout(FText::FromString(TEXT("Nut")), TurnsValue).ToString();
		TestTrue(TEXT("Turns readout includes the action label"), TurnsText.Contains(TEXT("Nut")));
		TestTrue(TEXT("Turns readout with max includes a span separator"), TurnsText.Contains(TEXT("/")));

		FDIVEInteractionValue CmValue;
		CmValue.Unit = EDIVEInteractionValueUnit::Centimeters;
		CmValue.Absolute = 1.25f;
		CmValue.AbsoluteMax = 5.f;
		const FString CmText =
			DIVE::FormatInteractionValueReadout(FText::FromString(TEXT("Slide")), CmValue).ToString();
		TestTrue(TEXT("Centimeters readout includes the unit suffix"), CmText.Contains(TEXT("cm")));
		TestTrue(TEXT("Centimeters readout with max includes a span separator"), CmText.Contains(TEXT("/")));

		FDIVEInteractionValue DomainValue;
		DomainValue.Unit = EDIVEInteractionValueUnit::None;
		DomainValue.Absolute = 2.5f;
		DomainValue.AbsoluteMax = 5.f;
		DomainValue.DisplaySuffix = FText::FromString(TEXT("A"));
		const FString DomainText =
			DIVE::FormatInteractionValueReadout(FText::FromString(TEXT("Current")), DomainValue).ToString();
		TestTrue(TEXT("Domain readout includes the action label"), DomainText.Contains(TEXT("Current")));
		TestTrue(TEXT("Domain readout includes the free suffix"), DomainText.Contains(TEXT("A")));
		TestTrue(TEXT("Domain readout with AbsoluteMax includes a span separator"), DomainText.Contains(TEXT("/")));
	}

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
	Context.PickHit.bBlockingHit = true;
	Context.PickHit.ImpactPoint = FVector(10.f, 0.f, 0.f);

	UDIVERotaryDriveAction* Rotary = NewObject<UDIVERotaryDriveAction>();
	TestNotNull(TEXT("Rotary action"), Rotary);
	if (Rotary)
	{
		Rotary->DegreesPerPixel = 1.f;
		Rotary->bLimitAngle = false;
		TestTrue(TEXT("Rotary CanExecute on kinematic primitive"), Rotary->CanExecute(Context));
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin"), Rotary->BeginInteraction(Context));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(0.f));
		{
			FDIVEInteractionUpdate HorizontalOnly = MakeZUpPolarUpdate(0.f);
			HorizontalOnly.ScreenDelta = FVector2D(10.f, 0.f);
			Rotary->UpdateInteraction(HorizontalOnly);
			TestTrue(
				TEXT("Rotary ignores horizontal ScreenDelta while the polar ray stays on the rim"),
				FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, 0.f, 0.05f));
		}
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(-10.f));
		TestTrue(
			TEXT("Rotary accumulated yaw from Z-up mapping"),
			FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, -10.f, 0.05f));
		{
			FDIVEInteractionUpdate Grazing;
			Grazing.ViewLocation = FVector::ZeroVector;
			Grazing.PickRayDir = FVector::ForwardVector;
			Grazing.DeltaTime = 0.016f;
			Rotary->UpdateInteraction(Grazing);
			Rotary->UpdateInteraction(MakeZUpPolarUpdate(20.f));
			TestTrue(
				TEXT("Rotary re-seeds after a lost rim hit instead of jumping"),
				FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, -10.f, 0.05f));
		}
		{
			const FDIVEInteractionValue RotaryValue = Rotary->MakeInteractionValue();
			TestEqual(
				TEXT("Rotary HUD unit is Degrees"),
				RotaryValue.Unit,
				EDIVEInteractionValueUnit::Degrees);
			TestTrue(
				TEXT("Rotary Absolute keeps signed accumulated degrees"),
				FMath::IsNearlyEqual(RotaryValue.Absolute, -10.f, 0.05f));
			TestTrue(
				TEXT("Unlimited rotary Normalized stays in 0..1"),
				RotaryValue.Normalized >= 0.f && RotaryValue.Normalized <= 1.f);
		}
		Rotary->EndInteraction(false);
		TestTrue(
			TEXT("Rotary cancel restores start rotation"),
			Sphere->GetRelativeRotation().IsNearlyZero(0.05f));

		Rotary->bLimitAngle = true;
		Rotary->MinAngleDegrees = 0.f;
		Rotary->MaxAngleDegrees = 90.f;
		Rotary->bUseDomainReadout = true;
		Rotary->DomainMin = 0.f;
		Rotary->DomainMax = 5.f;
		Rotary->ReadoutSuffix = FText::FromString(TEXT("A"));
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin for domain readout"), Rotary->BeginInteraction(Context));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(0.f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(45.f));
		{
			const FDIVEInteractionValue DomainRotary = Rotary->MakeInteractionValue();
			TestEqual(
				TEXT("Domain rotary Unit is None"),
				DomainRotary.Unit,
				EDIVEInteractionValueUnit::None);
			TestTrue(
				TEXT("Domain rotary Absolute lerps Normalized onto DomainMin..Max"),
				FMath::IsNearlyEqual(DomainRotary.Absolute, 2.5f, 0.1f));
			TestTrue(
				TEXT("Domain rotary AbsoluteMax is domain span"),
				FMath::IsNearlyEqual(DomainRotary.AbsoluteMax, 5.f, 0.05f));
			TestTrue(
				TEXT("Domain rotary DisplaySuffix is authored"),
				DomainRotary.DisplaySuffix.ToString() == TEXT("A"));
			const FString DomainRotaryHud =
				DIVE::FormatInteractionValueReadout(FText::FromString(TEXT("Current")), DomainRotary)
					.ToString();
			TestTrue(TEXT("Domain rotary HUD includes suffix"), DomainRotaryHud.Contains(TEXT("A")));
		}
		Rotary->EndInteraction(false);
		Rotary->bUseDomainReadout = false;
		Rotary->bLimitAngle = false;

		Rotary->DetentStepDegrees = 10.f;
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin for detent"), Rotary->BeginInteraction(Context));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(0.f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(-4.f));
		TestTrue(
			TEXT("Rotary detent holds the start snap across a sub-step move"),
			FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, 0.f, 0.05f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(-7.f));
		TestTrue(
			TEXT("Rotary detent snaps once raw angle crosses half a step"),
			FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, -10.f, 0.05f));
		Rotary->EndInteraction(false);
		Rotary->DetentStepDegrees = 0.f;

		Sphere->SetSimulatePhysics(true);
		TestTrue(
			TEXT("Rotary CanExecute stays true when Simulate Physics is on"),
			Rotary->CanExecute(Context));
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin forces kinematic"), Rotary->BeginInteraction(Context));
		TestFalse(
			TEXT("Begin cleared Simulate Physics"),
			Sphere->IsSimulatingPhysics());
		Rotary->EndInteraction(false);
		Sphere->SetSimulatePhysics(false);

		Sphere->SetRelativeTransform(FTransform::Identity);
		Rotary->bLimitAngle = true;
		Rotary->MinAngleDegrees = -180.f;
		Rotary->MaxAngleDegrees = 180.f;
		Rotary->bUseDomainReadout = false;
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin for rest-pose limits"), Rotary->BeginInteraction(Context));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(0.f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(180.f));
		TestTrue(
			TEXT("Rotary first gesture reaches the rest-relative MaxAngle"),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Sphere->GetRelativeRotation().Yaw, 180.f)) < 0.05f);
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(270.f));
		TestTrue(
			TEXT("Rotary discards extra polar travel past MaxAngle"),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Sphere->GetRelativeRotation().Yaw, 180.f)) < 0.05f);
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(260.f));
		TestTrue(
			TEXT("Rotary reverse from the stop does not unwind discarded overshoot"),
			FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, 170.f, 0.05f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(270.f));
		TestTrue(
			TEXT("Rotary can return to MaxAngle after leaving the stop"),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Sphere->GetRelativeRotation().Yaw, 180.f)) < 0.05f);
		Rotary->EndInteraction(true);
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary re-Begin after committing the limit"), Rotary->BeginInteraction(Context));
		TestTrue(
			TEXT("Rotary re-Begin does not snap the committed ±180 pose back to rest"),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Sphere->GetRelativeRotation().Yaw, 180.f)) < 0.05f);
		{
			const FDIVEInteractionValue PersistedRotary = Rotary->MakeInteractionValue();
			TestTrue(
				TEXT("Rotary HUD keeps the committed angle across mouse-up"),
				FMath::IsNearlyEqual(PersistedRotary.Absolute, 180.f, 0.05f));
		}
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(0.f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(180.f));
		TestTrue(
			TEXT("Rotary cannot walk past MaxAngle by releasing and grabbing again"),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Sphere->GetRelativeRotation().Yaw, 180.f)) < 0.05f);
		TestTrue(
			TEXT("Rotary Absolute stays at the rest-relative limit after a second 180 drag"),
			FMath::IsNearlyEqual(Rotary->MakeInteractionValue().Absolute, 180.f, 0.05f));
		Rotary->EndInteraction(true);
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin to leave the committed stop"), Rotary->BeginInteraction(Context));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(180.f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(90.f));
		TestTrue(
			TEXT("Rotary can leave the stop toward rest in a later gesture"),
			FMath::IsNearlyEqual(Sphere->GetRelativeRotation().Yaw, 90.f, 0.05f));
		Rotary->EndInteraction(false);
		TestTrue(
			TEXT("Rotary cancel restores the committed pose, not rest"),
			FMath::Abs(FMath::FindDeltaAngleDegrees(Sphere->GetRelativeRotation().Yaw, 180.f)) < 0.05f);

		Sphere->SetRelativeTransform(FTransform::Identity);
		Rotary->MinAngleDegrees = 0.f;
		Rotary->MaxAngleDegrees = 90.f;
		Rotary->bUseDomainReadout = true;
		Rotary->DomainMin = 0.f;
		Rotary->DomainMax = 5.f;
		Rotary->ReadoutSuffix = FText::FromString(TEXT("A"));
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary Begin for persisted domain readout"), Rotary->BeginInteraction(Context));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(0.f));
		Rotary->UpdateInteraction(MakeZUpPolarUpdate(45.f));
		Rotary->EndInteraction(true);
		Rotary->MarkInteractionActive(Context);
		TestTrue(TEXT("Rotary re-Begin after committing a domain value"), Rotary->BeginInteraction(Context));
		{
			const FDIVEInteractionValue PersistedDomain = Rotary->MakeInteractionValue();
			TestTrue(
				TEXT("Domain rotary Absolute persists across gestures"),
				FMath::IsNearlyEqual(PersistedDomain.Absolute, 2.5f, 0.1f));
		}
		Rotary->EndInteraction(true);
		Rotary->bUseDomainReadout = false;
		Rotary->bLimitAngle = false;
		Sphere->SetRelativeTransform(FTransform::Identity);
	}

	UDIVEThreadedDriveAction* Threaded = NewObject<UDIVEThreadedDriveAction>();
	TestNotNull(TEXT("Threaded action"), Threaded);
	if (Threaded)
	{
		Threaded->Axis = EDIVEDriveAxis::X;
		Threaded->DegreesPerPixel = 1.f;
		Threaded->TurnsToRelease = 1.f;
		Threaded->PitchCmPerTurn = 2.f;
		Context.PickHit.ImpactPoint = FVector(0.f, 10.f, 0.f);
		Threaded->MarkInteractionActive(Context);
		TestTrue(TEXT("Threaded Begin"), Threaded->BeginInteraction(Context));
		Threaded->UpdateInteraction(MakeXAxisPolarUpdate(0.f));
		Threaded->UpdateInteraction(MakeXAxisPolarUpdate(180.f));
		TestTrue(
			TEXT("Threaded half-turn translates along local X"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 1.f, 0.05f));
		{
			const FDIVEInteractionValue ThreadedValue = Threaded->MakeInteractionValue();
			TestEqual(
				TEXT("Threaded HUD unit is Turns"),
				ThreadedValue.Unit,
				EDIVEInteractionValueUnit::Turns);
			TestTrue(
				TEXT("Threaded Absolute is accumulated turns"),
				FMath::IsNearlyEqual(ThreadedValue.Absolute, 0.5f, 0.05f));
			TestTrue(
				TEXT("Threaded AbsoluteMax is TurnsToRelease"),
				FMath::IsNearlyEqual(ThreadedValue.AbsoluteMax, 1.f, 0.05f));
		}
		Threaded->EndInteraction(false);
		TestTrue(
			TEXT("Threaded cancel restores start transform"),
			Sphere->GetRelativeTransform().Equals(FTransform::Identity, 0.05f));

		Sphere->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		const FVector ScrewAxisWorld = Sphere->GetComponentTransform().TransformVectorNoScale(FVector::ForwardVector);
		Context.PickHit.ImpactPoint = FVector(10.f, 0.f, 0.f);
		Threaded->MarkInteractionActive(Context);
		TestTrue(TEXT("Threaded Begin with pre-rotated part"), Threaded->BeginInteraction(Context));
		Threaded->UpdateInteraction(MakePolarUpdateAroundAxis(ScrewAxisWorld, 180.f));
		Threaded->UpdateInteraction(MakePolarUpdateAroundAxis(ScrewAxisWorld, 270.f));
		Threaded->UpdateInteraction(MakePolarUpdateAroundAxis(ScrewAxisWorld, 360.f));
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

		Sphere->SetRelativeTransform(FTransform::Identity);
		Threaded->TurnsToRelease = 4.f;
		Threaded->PitchCmPerTurn = 2.f;
		Context.PickHit.ImpactPoint = FVector(0.f, 10.f, 0.f);
		Threaded->MarkInteractionActive(Context);
		TestTrue(TEXT("Threaded Begin for rest-pose turns"), Threaded->BeginInteraction(Context));
		Threaded->UpdateInteraction(MakeXAxisPolarUpdate(0.f));
		Threaded->UpdateInteraction(MakeXAxisPolarUpdate(360.f));
		TestTrue(
			TEXT("Threaded first gesture commits one turn from rest"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 2.f, 0.05f));
		Threaded->EndInteraction(true);
		Threaded->MarkInteractionActive(Context);
		TestTrue(TEXT("Threaded re-Begin after committing a turn"), Threaded->BeginInteraction(Context));
		TestTrue(
			TEXT("Threaded HUD keeps committed turns across mouse-up"),
			FMath::IsNearlyEqual(Threaded->MakeInteractionValue().Absolute, 1.f, 0.05f));
		Threaded->UpdateInteraction(MakeXAxisPolarUpdate(0.f));
		Threaded->UpdateInteraction(MakeXAxisPolarUpdate(360.f));
		TestTrue(
			TEXT("Threaded continues from rest instead of resetting on re-grab"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 4.f, 0.05f));
		TestTrue(
			TEXT("Threaded Absolute is two turns after the second gesture"),
			FMath::IsNearlyEqual(Threaded->MakeInteractionValue().Absolute, 2.f, 0.05f));
		Threaded->EndInteraction(false);
		TestTrue(
			TEXT("Threaded cancel restores the committed turn pose, not rest"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 2.f, 0.05f));
		Sphere->SetRelativeTransform(FTransform::Identity);
	}

	UDIVELinearDriveAction* Linear = NewObject<UDIVELinearDriveAction>();
	TestNotNull(TEXT("Linear action"), Linear);
	if (Linear)
	{
		Sphere->SetRelativeTransform(FTransform::Identity);
		Linear->Axis = EDIVEDriveAxis::X;
		Linear->CmPerPixel = 1.f;
		Linear->bLimitTravel = true;
		Linear->MinTravelCm = 0.f;
		Linear->MaxTravelCm = 10.f;
		Context.PickHit.ImpactPoint = FVector::ZeroVector;
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear Begin"), Linear->BeginInteraction(Context));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.f));
		{
			FDIVEInteractionUpdate HorizontalOnly = MakeXAxisLinearUpdate(0.f);
			HorizontalOnly.ScreenDelta = FVector2D(10.f, 0.f);
			Linear->UpdateInteraction(HorizontalOnly);
			TestTrue(
				TEXT("Linear ignores horizontal ScreenDelta while the travel ray stays on the rail"),
				FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 0.f, 0.05f));
		}
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(5.f));
		TestTrue(
			TEXT("Linear accumulated travel along local X"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 5.f, 0.05f));
		{
			const FDIVEInteractionValue LinearValue = Linear->MakeInteractionValue();
			TestEqual(
				TEXT("Linear HUD unit is Centimeters"),
				LinearValue.Unit,
				EDIVEInteractionValueUnit::Centimeters);
			TestTrue(
				TEXT("Linear Absolute is accumulated centimetres"),
				FMath::IsNearlyEqual(LinearValue.Absolute, 5.f, 0.05f));
			TestTrue(
				TEXT("Linear AbsoluteMax is travel span"),
				FMath::IsNearlyEqual(LinearValue.AbsoluteMax, 10.f, 0.05f));
		}
		Linear->bUseDomainReadout = true;
		Linear->DomainMin = 0.f;
		Linear->DomainMax = 20.f;
		Linear->ReadoutSuffix = FText::FromString(TEXT("V"));
		{
			const FDIVEInteractionValue DomainLinear = Linear->MakeInteractionValue();
			TestEqual(
				TEXT("Domain linear Unit is None"),
				DomainLinear.Unit,
				EDIVEInteractionValueUnit::None);
			TestTrue(
				TEXT("Domain linear Absolute lerps mid-travel onto DomainMin..Max"),
				FMath::IsNearlyEqual(DomainLinear.Absolute, 10.f, 0.1f));
			TestTrue(
				TEXT("Domain linear AbsoluteMax is domain span"),
				FMath::IsNearlyEqual(DomainLinear.AbsoluteMax, 20.f, 0.05f));
			TestTrue(
				TEXT("Domain linear DisplaySuffix is authored"),
				DomainLinear.DisplaySuffix.ToString() == TEXT("V"));
		}
		Linear->bUseDomainReadout = false;
		{
			FDIVEInteractionUpdate Parallel;
			Parallel.ViewLocation = FVector::ZeroVector;
			Parallel.PickRayDir = FVector::ForwardVector;
			Parallel.DeltaTime = 0.016f;
			Linear->UpdateInteraction(Parallel);
			Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.f));
			TestTrue(
				TEXT("Linear re-seeds after a lost rail projection instead of jumping"),
				FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 5.f, 0.05f));
		}
		Linear->EndInteraction(false);
		TestTrue(
			TEXT("Linear cancel restores start transform"),
			Sphere->GetRelativeTransform().Equals(FTransform::Identity, 0.05f));

		Linear->DetentStepCm = 1.f;
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear Begin for detent"), Linear->BeginInteraction(Context));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.4f));
		TestTrue(
			TEXT("Linear detent holds the start snap across a sub-step move"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 0.f, 0.05f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.7f));
		TestTrue(
			TEXT("Linear detent snaps once raw travel crosses half a step"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 1.f, 0.05f));
		Linear->EndInteraction(false);

		Linear->DetentStepCm = 0.f;
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear Begin for limit return"), Linear->BeginInteraction(Context));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(15.f));
		TestTrue(
			TEXT("Linear clamps travel at MaxTravelCm"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 10.f, 0.05f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(14.f));
		TestTrue(
			TEXT("Linear reverse from the stop does not unwind discarded overshoot"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 9.f, 0.05f));
		Linear->EndInteraction(false);

		Sphere->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear Begin with pre-rotated part"), Linear->BeginInteraction(Context));
		Linear->UpdateInteraction(MakeLinearUpdateAlongAxis(
			Sphere->GetComponentTransform().TransformVectorNoScale(FVector::ForwardVector),
			0.f));
		Linear->UpdateInteraction(MakeLinearUpdateAlongAxis(
			Sphere->GetComponentTransform().TransformVectorNoScale(FVector::ForwardVector),
			5.f));
		TestTrue(
			TEXT("Linear travel follows local X in parent space"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().Y, 5.f, 0.05f));
		TestTrue(
			TEXT("Linear travel does not slide along parent X for a yaw-90 part"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 0.f, 0.05f));
		Linear->EndInteraction(false);
		TestTrue(
			TEXT("Linear cancel restores pre-rotated start transform"),
			Sphere->GetRelativeTransform().Equals(FTransform(FRotator(0.f, 90.f, 0.f)), 0.05f));
		Sphere->SetRelativeTransform(FTransform::Identity);

		Linear->bLimitTravel = true;
		Linear->MinTravelCm = 0.f;
		Linear->MaxTravelCm = 10.f;
		Linear->bUseDomainReadout = false;
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear Begin for rest-pose travel"), Linear->BeginInteraction(Context));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(10.f));
		TestTrue(
			TEXT("Linear first gesture reaches the rest-relative MaxTravel"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 10.f, 0.05f));
		Linear->EndInteraction(true);
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear re-Begin after committing the stop"), Linear->BeginInteraction(Context));
		TestTrue(
			TEXT("Linear re-Begin does not snap the committed stop back to rest"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 10.f, 0.05f));
		TestTrue(
			TEXT("Linear HUD keeps committed travel across mouse-up"),
			FMath::IsNearlyEqual(Linear->MakeInteractionValue().Absolute, 10.f, 0.05f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(0.f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(10.f));
		TestTrue(
			TEXT("Linear cannot walk past MaxTravel by releasing and grabbing again"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 10.f, 0.05f));
		Linear->EndInteraction(true);
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear Begin to leave the committed stop"), Linear->BeginInteraction(Context));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(10.f));
		Linear->UpdateInteraction(MakeXAxisLinearUpdate(4.f));
		TestTrue(
			TEXT("Linear can leave the stop toward rest in a later gesture"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 4.f, 0.05f));
		Linear->EndInteraction(false);
		TestTrue(
			TEXT("Linear cancel restores the committed pose, not rest"),
			FMath::IsNearlyEqual(Sphere->GetRelativeLocation().X, 10.f, 0.05f));

		Linear->bUseDomainReadout = true;
		Linear->DomainMin = 0.f;
		Linear->DomainMax = 5.f;
		Linear->ReadoutSuffix = FText::FromString(TEXT("V"));
		Linear->MarkInteractionActive(Context);
		TestTrue(TEXT("Linear re-Begin after committing a domain stop"), Linear->BeginInteraction(Context));
		{
			const FDIVEInteractionValue PersistedDomain = Linear->MakeInteractionValue();
			TestTrue(
				TEXT("Domain linear Absolute persists across gestures"),
				FMath::IsNearlyEqual(PersistedDomain.Absolute, 5.f, 0.1f));
		}
		Linear->EndInteraction(true);
		Linear->bUseDomainReadout = false;
		Sphere->SetRelativeTransform(FTransform::Identity);
	}

	{
		ADIVETestPhysicalDrivePawn* PhysicalHost = SpawnSmokeHost(World);
		TestNotNull(TEXT("Physical-device host"), PhysicalHost);
		if (PhysicalHost)
		{
			USphereComponent* DeviceBody = NewObject<USphereComponent>(PhysicalHost, TEXT("DeviceBody"));
			DeviceBody->SetSphereRadius(24.f);
			DeviceBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			DeviceBody->SetSimulatePhysics(true);
			PhysicalHost->SetRootComponent(DeviceBody);
			PhysicalHost->AddInstanceComponent(DeviceBody);
			DeviceBody->RegisterComponent();

			USphereComponent* Knob = NewObject<USphereComponent>(PhysicalHost, TEXT("DeviceKnob"));
			Knob->SetSphereRadius(8.f);
			Knob->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Knob->SetupAttachment(DeviceBody);
			PhysicalHost->AddInstanceComponent(Knob);
			Knob->RegisterComponent();

			FDIVEActionContext KnobContext;
			KnobContext.Target = Knob;
			KnobContext.ViewRotation = FRotator::ZeroRotator;
			KnobContext.PickHit.bBlockingHit = true;
			KnobContext.PickHit.ImpactPoint = FVector(8.f, 0.f, 0.f);

			UDIVERotaryDriveAction* IsolatedRotary = NewObject<UDIVERotaryDriveAction>();
			TestNotNull(TEXT("Rotary on a simulating parent"), IsolatedRotary);
			if (!IsolatedRotary)
			{
				PhysicalHost->Destroy();
				return false;
			}

			IsolatedRotary->DegreesPerPixel = 1.f;
			IsolatedRotary->bLimitAngle = false;
			IsolatedRotary->MarkInteractionActive(KnobContext);

			const FVector BodyBefore = DeviceBody->GetComponentLocation();
			TestTrue(TEXT("Rotary Begin on a child of a simulating body"), IsolatedRotary->BeginInteraction(KnobContext));
			TestTrue(TEXT("Begin leaves the parent simulating"), DeviceBody->BodyInstance.bSimulatePhysics);
			TestFalse(TEXT("Begin unwelded the driven child"), Knob->IsWelded());
			TestEqual(
				TEXT("Begin sets Query Only on the driven child"),
				Knob->GetCollisionEnabled(),
				ECollisionEnabled::QueryOnly);

			// First Update only seeds the polar angle; isolation is the assert here.
			IsolatedRotary->UpdateInteraction(MakeZUpPolarUpdate(-30.f));
			TestTrue(
				TEXT("Rotary on a welded child does not launch the simulating parent"),
				DeviceBody->GetComponentLocation().Equals(BodyBefore, 2.f));

			IsolatedRotary->EndInteraction(true);
			PhysicalHost->Destroy();
		}
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
	FDIVEPhysicalGrabOnlySmokeTest,
	"DIVE.PawnPhysicalDrive.PhysicalGrabOnly",
	SmokeUnitTestFlags)

bool FDIVEPhysicalGrabOnlySmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestNull(
		TEXT("InternalProxyDriveAction property removed from session"),
		FindFProperty<FProperty>(UDIVESessionSubsystem::StaticClass(), TEXT("InternalProxyDriveAction")));

	const UClass* ForwardClass = UDIVEProxyDriveForwardAction::StaticClass();
	TestNotNull(TEXT("ForwardAction class"), ForwardClass);
	if (ForwardClass)
	{
		TestFalse(
			TEXT("ForwardAction is authorable in Catalog (not HideDropdown)"),
			ForwardClass->HasAnyClassFlags(CLASS_HideDropDown));
		TestTrue(
			TEXT("ForwardAction is BlueprintType / EditInlineNew continuous"),
			ForwardClass->IsChildOf(UDIVEContinuousDeviceAction::StaticClass()));
	}

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
		AddInfo(TEXT("No Game/PIE world — skipped Physical routing asserts."));
		return true;
	}

	UDIVESessionSubsystem* Subsystem = World->GetSubsystem<UDIVESessionSubsystem>();
	APlayerController* PC = DIVEPlayerQuery::FindLocalPlayerController(World);
	if (!Subsystem || !PC)
	{
		AddInfo(TEXT("No session subsystem or local PC — skipped Physical routing asserts."));
		return true;
	}

	if (Subsystem->IsSessionActive())
	{
		AddInfo(TEXT("A DIVE session is already active — skipped to avoid clobbering PIE."));
		return true;
	}

	ADIVETestPhysicalDrivePawn* Device = SpawnSmokeHost(World);
	TestNotNull(TEXT("Device host"), Device);
	if (!Device)
	{
		return false;
	}

	UDIVEInspectableComponent* Inspectable = NewObject<UDIVEInspectableComponent>(
		Device,
		UDIVEInspectableComponent::StaticClass(),
		TEXT("DIVEInspectable"));
	UDIVETestProxyDriveComponent* Drive = NewObject<UDIVETestProxyDriveComponent>(Device, TEXT("TestProxy"));
	USphereComponent* HitSphere = MakeSphereOnHost(Device, TEXT("ProxyHit"), 16.f);
	if (!Inspectable || !Drive || !HitSphere)
	{
		Device->Destroy();
		return false;
	}

	Device->AddInstanceComponent(Inspectable);
	Device->AddInstanceComponent(Drive);
	if (!Inspectable->IsRegistered())
	{
		Inspectable->RegisterComponent();
	}
	if (!Drive->IsRegistered())
	{
		Drive->RegisterComponent();
	}

	TestTrue(
		TEXT("Proxy resolves on the hit sphere"),
		DIVEProxyDriveResolve::FindProxyDriveForHit(HitSphere) != nullptr);

	if (!Subsystem->TryBeginSession(Device, Inspectable, FDIVESessionParams()))
	{
		AddInfo(TEXT("TryBeginSession failed — skipped Physical routing."));
		Device->Destroy();
		return true;
	}

	Subsystem->SetInteractionMode(EDIVESessionInteractionMode::Physical);

	// Physical must never start a continuous/proxy gesture. Pawn GRIP may still succeed when a
	// physical-drive backend exists — that is grab, not device proxy.
	const bool bBegan = Subsystem->TryBeginProxyDriveAtScreenPosition(FVector2D(100.f, 100.f), PC);
	if (bBegan)
	{
		TestTrue(
			TEXT("Physical begin that succeeds is pawn GRIP (not continuous/proxy)"),
			Subsystem->IsPawnPhysicalDriveActive());
		TestTrue(TEXT("Pawn GRIP reports proxy-driving flag"), Subsystem->IsProxyDriving());
		Subsystem->EndProxyDrive(true);
		TestFalse(TEXT("Drive cleared after End"), Subsystem->IsProxyDriving());
	}
	else
	{
		TestFalse(TEXT("No drive left active after failed Physical begin"), Subsystem->IsProxyDriving());
		AddInfo(TEXT("Physical begin failed (no pick and/or no pawn drive) — proxy auto-start still ruled out."));
	}

	Subsystem->EndSession(EDIVESessionEndReason::Forced);
	Device->Destroy();
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
