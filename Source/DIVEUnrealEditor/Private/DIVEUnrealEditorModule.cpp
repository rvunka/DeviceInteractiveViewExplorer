// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceScan.h"
#include "Debug/DIVEDebugDump.h"

#include "Editor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Modules/ModuleManager.h"
#include "Selection.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DIVEUnrealEditor"

namespace
{
void NotifyDumpResult(const FString& FilePath)
{
	FNotificationInfo Info(FilePath.IsEmpty()
		? LOCTEXT("DumpFailed", "DIVE dump failed — check Output Log (LogDIVE).")
		: FText::FromString(FString::Printf(TEXT("DIVE dump saved:\n%s"), *FilePath)));
	Info.ExpireDuration = 6.0f;
	Info.bUseLargeFont = false;
	FSlateNotificationManager::Get().AddNotification(Info);
}

void ExecuteScanSelectedActors()
{
	if (!GEditor)
	{
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors)
	{
		return;
	}

	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			const FDIVEDeviceScanReport Report = DIVEDeviceScan::ScanActor(Actor);
			UE_LOG(LogTemp, Display, TEXT("%s"), *Report.ToLogString());
		}
	}
}

void ExecuteDumpSelectedActors()
{
	if (!GEditor)
	{
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (!SelectedActors)
	{
		return;
	}

	FString LastPath;
	for (FSelectionIterator It(*SelectedActors); It; ++It)
	{
		if (AActor* Actor = Cast<AActor>(*It))
		{
			DIVEDebugDump::DumpDevice(Actor);
			LastPath = DIVEDebugDump::GetLastDumpFilePath();
		}
	}

	NotifyDumpResult(LastPath);
}
}

class FDIVEUnrealEditorModule : public IModuleInterface
{
public:
	void StartupModule() override
	{
		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this,
			&FDIVEUnrealEditorModule::RegisterMenus));
	}

	void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);
	}

private:
	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(UE_MODULE_NAME);
		if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContext"))
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("DIVE");
			Section.AddMenuEntry(
				"DIVE_ScanDevice",
				LOCTEXT("ScanDeviceLabel", "DIVE Scan Device"),
				LOCTEXT("ScanDeviceTooltip", "Validate anchors and pick context menu catalog on the selected device actor."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&ExecuteScanSelectedActors)));
			Section.AddMenuEntry(
				"DIVE_DumpDevice",
				LOCTEXT("DumpDeviceLabel", "DIVE Dump Device"),
				LOCTEXT("DumpDeviceTooltip", "Dump catalog keys vs component FNames / Handle_* / Is_* to Output Log and Saved/DIVE/Dumps (cyan on-screen toast)."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&ExecuteDumpSelectedActors)));
		}
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDIVEUnrealEditorModule, DIVEUnrealEditor)
