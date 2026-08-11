// Copyright (c) 2026. All Rights Reserved.

#include "DIVEDeviceScan.h"
#include "DIVELog.h"
#include "Debug/DIVEDebugDump.h"
#include "DIVEEditorAssetCategory.h"
#include "AssetTypeActions_DIVEActionCatalog.h"
#include "Customizations/DIVEActionBindingCustomization.h"
#include "DIVEActionBinding.h"

#include "AssetToolsModule.h"
#include "Editor.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Selection.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "DIVEUnrealEditor"

namespace
{
EAssetTypeCategories::Type GDIVEAssetCategory = EAssetTypeCategories::Misc;
TSharedPtr<FAssetTypeActions_DIVEActionCatalog> GDIVEActionCatalogAssetTypeActions;

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
			UE_LOG(LogDIVE, Display, TEXT("%s"), *Report.ToLogString());
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

namespace DIVEEditor
{
EAssetTypeCategories::Type GetDIVEAssetCategory()
{
	return GDIVEAssetCategory;
}

void SetDIVEAssetCategory(EAssetTypeCategories::Type Category)
{
	GDIVEAssetCategory = Category;
}
}

class FDIVEUnrealEditorModule : public IModuleInterface
{
public:
	void StartupModule() override
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		DIVEEditor::SetDIVEAssetCategory(AssetTools.RegisterAdvancedAssetCategory(
			FName(TEXT("DIVE")),
			DIVEEditor::GetDIVECategoryText()));

		GDIVEActionCatalogAssetTypeActions = MakeShared<FAssetTypeActions_DIVEActionCatalog>();
		AssetTools.RegisterAssetTypeActions(GDIVEActionCatalogAssetTypeActions.ToSharedRef());

		FPropertyEditorModule& PropertyEditor =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditor.RegisterCustomPropertyTypeLayout(
			FDIVEActionBinding::StaticStruct()->GetFName(),
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(
				&FDIVEActionBindingCustomization::MakeInstance));
		PropertyEditor.NotifyCustomizationModuleChanged();

		UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(
			this,
			&FDIVEUnrealEditorModule::RegisterMenus));
	}

	void ShutdownModule() override
	{
		UToolMenus::UnRegisterStartupCallback(this);
		UToolMenus::UnregisterOwner(this);

		if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
		{
			FPropertyEditorModule& PropertyEditor =
				FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyEditor.UnregisterCustomPropertyTypeLayout(
				FDIVEActionBinding::StaticStruct()->GetFName());
			PropertyEditor.NotifyCustomizationModuleChanged();
		}

		if (FModuleManager::Get().IsModuleLoaded("AssetTools") && GDIVEActionCatalogAssetTypeActions.IsValid())
		{
			IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
			AssetTools.UnregisterAssetTypeActions(GDIVEActionCatalogAssetTypeActions.ToSharedRef());
		}
		GDIVEActionCatalogAssetTypeActions.Reset();
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
				LOCTEXT("ScanDeviceTooltip", "Validate anchors, Catalog / Bindings on the selected device actor."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&ExecuteScanSelectedActors)));
			Section.AddMenuEntry(
				"DIVE_DumpDevice",
				LOCTEXT("DumpDeviceLabel", "DIVE Dump Device"),
				LOCTEXT("DumpDeviceTooltip", "Dump action bindings, sections, and resolved picks to Output Log and Saved/DIVE/Dumps."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateStatic(&ExecuteDumpSelectedActors)));
		}
	}
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FDIVEUnrealEditorModule, DIVEUnrealEditor)
