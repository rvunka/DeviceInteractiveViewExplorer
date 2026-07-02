// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceScan.h"

#include "Editor.h"
#include "Modules/ModuleManager.h"
#include "Selection.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "DIVEUnrealEditor"

namespace
{
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
		}
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDIVEUnrealEditorModule, DIVEUnrealEditor)
