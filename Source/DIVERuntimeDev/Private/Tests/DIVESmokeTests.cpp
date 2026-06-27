#if WITH_DEV_AUTOMATION_TESTS

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

#endif // WITH_DEV_AUTOMATION_TESTS
