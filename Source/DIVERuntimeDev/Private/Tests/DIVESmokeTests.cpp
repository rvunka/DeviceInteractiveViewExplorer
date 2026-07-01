#if WITH_DEV_AUTOMATION_TESTS

#include "DIVEConvention.h"
#include "DIVEPawnPhysicalDriveResolve.h"
#include "DIVEProxyDriveResolve.h"
#include "DIVETypes.h"
#include "Utils/DIVEContextMenu.h"

#include "Misc/AutomationTest.h"

namespace
{
	// UnrealEditor-Cmd does not register TypedElement "Components"; skip commandlet context.
	constexpr EAutomationTestFlags SmokeUnitTestFlags =
		EAutomationTestFlags::EditorContext
		| EAutomationTestFlags::ClientContext
		| EAutomationTestFlags::SmokeFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEContextMenuBuiltInEntriesSmokeTest,
	"DIVE.ContextMenu.BuiltInEntries",
	SmokeUnitTestFlags)

bool FDIVEContextMenuBuiltInEntriesSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FDIVEContextMenuEntry> Entries;
	FDIVEFocusTarget PickTarget = FDIVEFocusTarget::FromPrimitive(nullptr, NAME_None);

	DIVEContextMenu::BuildStandardEntries(nullptr, PickTarget, false, Entries);
	TestEqual(TEXT("Standard entries without pick"), Entries.Num(), 2);
	TestEqual(TEXT("Focus entry id"), Entries[0].ActionId, DIVE::kContextFocus);
	TestFalse(TEXT("Focus disabled without pick"), Entries[0].bEnabled);
	TestFalse(TEXT("Isolate disabled without pick"), Entries[1].bEnabled);

	Entries.Reset();
	const FDIVEFocusTarget DeviceRootPick = FDIVEFocusTarget::MakeDeviceRoot();
	DIVEContextMenu::BuildStandardEntries(nullptr, DeviceRootPick, true, Entries);
	TestEqual(TEXT("Device root has navigation only"), Entries.Num(), 2);
	TestFalse(TEXT("Focus disabled for device root pick"), Entries[0].bEnabled);
	TestFalse(TEXT("Isolate disabled for device root pick"), Entries[1].bEnabled);

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

#endif // WITH_DEV_AUTOMATION_TESTS
