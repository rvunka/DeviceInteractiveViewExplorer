#if WITH_DEV_AUTOMATION_TESTS

#include "Utils/DIVEManipulation.h"
#include "Utils/DIVEOperations.h"

#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEOperationValidationSmokeTest,
	"ATSEP.DIVE.Operations.ValidationRules",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FDIVEOperationValidationSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TSet<FName> Completed;
	FText FailureMessage;

	TestTrue(TEXT("Empty operation id fails validation"), !DIVEOperations::ValidateOperation(nullptr, NAME_None, Completed, FailureMessage));

	TestTrue(TEXT("Operation without rules passes"), DIVEOperations::ValidateOperation(nullptr, TEXT("Inspect"), Completed, FailureMessage));

	FDIVEOperationDescriptor Descriptor = DIVEOperations::MakeFallbackDescriptor(TEXT("Inspect"));
	TestEqual(TEXT("Fallback descriptor id"), Descriptor.OperationId, FName(TEXT("Inspect")));
	TestEqual(TEXT("Fallback input mode is Press"), Descriptor.InputMode, EDIVEOperationInputMode::Press);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FDIVEManipulationSnapSmokeTest,
	"ATSEP.DIVE.Manipulation.HingeSnap",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool FDIVEManipulationSnapSmokeTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const TArray<float> SnapAngles = {0.f, 45.f, 90.f};
	TestEqual(TEXT("Snap to 45"), DIVEManipulation::SnapAngle(50.f, SnapAngles), 45.f);
	TestEqual(TEXT("Snap to 0"), DIVEManipulation::SnapAngle(10.f, SnapAngles), 0.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
