#if WITH_DEV_AUTOMATION_TESTS

#include "DIVEConvention.h"
#include "Utils/DIVEContextMenu.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEContextMenuBuiltInEntriesSmokeTest,
	"DIVE.ContextMenu.BuiltInEntries",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FDIVEContextMenuBuiltInEntriesSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TArray<FDIVEContextMenuEntry> Entries;
	FDIVEFocusTarget PickTarget = FDIVEFocusTarget::FromPrimitive(nullptr, NAME_None);

	DIVEContextMenu::BuildBuiltInEntries(nullptr, PickTarget, false, Entries);
	TestEqual(TEXT("Built-in entries without pick"), Entries.Num(), 2);
	TestEqual(TEXT("Focus entry id"), Entries[0].ActionId, DIVE::kContextFocus);
	TestFalse(TEXT("Focus disabled without pick"), Entries[0].bEnabled);

	Entries.Reset();
	const FDIVEFocusTarget ValidPickTarget = FDIVEFocusTarget::MakeDeviceRoot();
	DIVEContextMenu::BuildBuiltInEntries(nullptr, ValidPickTarget, true, Entries);
	TestTrue(TEXT("Focus enabled with valid pick target"), Entries[0].bEnabled);

	TestEqual(TEXT("Back action id constant"), DIVE::kContextBack, FName(TEXT("DIVE.Context.Back")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
