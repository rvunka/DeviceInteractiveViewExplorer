// Copyright (c) 2026. All Rights Reserved.

#include "Customizations/DIVEActionBindingCustomization.h"

#include "DIVEActionCatalogAsset.h"
#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"

#include "BlueprintEditorModule.h"
#include "Components/PrimitiveComponent.h"
#include "Containers/Set.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "IDetailChildrenBuilder.h"
#include "Modules/ModuleManager.h"
#include "PropertyHandle.h"
#include "Selection.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DIVEActionBindingCustomization"

namespace
{
bool IsLiveWorldActor(const AActor* Actor)
{
	if (!Actor || Actor->HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return false;
	}

	const UWorld* World = Actor->GetWorld();
	return World
		&& (World->WorldType == EWorldType::Editor
			|| World->WorldType == EWorldType::EditorPreview
			|| World->WorldType == EWorldType::PIE);
}

bool IsLevelSelectableWorld(const UWorld* World)
{
	return World
		&& (World->WorldType == EWorldType::Editor || World->WorldType == EWorldType::PIE);
}

bool TrySelectMatchingInBlueprintEditor(const TArray<UPrimitiveComponent*>& Matching)
{
	FBlueprintEditorModule* Kismet = FModuleManager::GetModulePtr<FBlueprintEditorModule>(TEXT("Kismet"));
	if (!Kismet)
	{
		return false;
	}

	for (const TSharedRef<IBlueprintEditor>& Editor : Kismet->GetBlueprintEditors())
	{
		bool bSelectedAny = false;
		for (UPrimitiveComponent* Primitive : Matching)
		{
			if (!Primitive)
			{
				continue;
			}

			if (Editor->FindAndSelectSubobjectEditorTreeNode(Primitive, bSelectedAny).IsValid())
			{
				bSelectedAny = true;
			}
		}

		if (bSelectedAny)
		{
			return true;
		}
	}

	return false;
}
}

TSharedRef<IPropertyTypeCustomization> FDIVEActionBindingCustomization::MakeInstance()
{
	return MakeShared<FDIVEActionBindingCustomization>();
}

void FDIVEActionBindingCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	(void)StructCustomizationUtils;

	StructHandle = StructPropertyHandle;

	HeaderRow
		.NameContent()
		[
			SNew(STextBlock)
			.Text(this, &FDIVEActionBindingCustomization::GetCollapsedHeaderText)
			.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
		]
		.ValueContent()
		[
			StructPropertyHandle->CreatePropertyValueWidget()
		];
}

void FDIVEActionBindingCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	(void)StructCustomizationUtils;

	StructHandle = StructPropertyHandle;
	PrimaryIndexHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, PrimaryActionIndex));
	ActionsHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, Actions));
	TargetsHandle = StructPropertyHandle->GetChildHandle(
		GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, Targets));

	if (ActionsHandle.IsValid())
	{
		ActionsHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FDIVEActionBindingCustomization::RebuildPrimaryOptions));
		ActionsHandle->SetOnChildPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FDIVEActionBindingCustomization::RebuildPrimaryOptions));
	}

	if (TargetsHandle.IsValid())
	{
		TargetsHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FDIVEActionBindingCustomization::InvalidateMatchedPreview));
		TargetsHandle->SetOnChildPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FDIVEActionBindingCustomization::InvalidateMatchedPreview));
	}

	RebuildPrimaryOptions();

	int32 InitialPrimaryIndex = INDEX_NONE;
	if (PrimaryIndexHandle.IsValid())
	{
		PrimaryIndexHandle->GetValue(InitialPrimaryIndex);
	}

	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);
	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedPtr<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex);
		if (!ChildHandle.IsValid() || !ChildHandle->GetProperty())
		{
			continue;
		}

		const FName ChildName = ChildHandle->GetProperty()->GetFName();
		if (ChildName == GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, PrimaryActionIndex)
			&& PrimaryIndexHandle.IsValid())
		{
			TSharedRef<SComboBox<TSharedPtr<FPrimaryOption>>> PrimaryCombo =
				SNew(SComboBox<TSharedPtr<FPrimaryOption>>)
				.OptionsSource(&PrimaryOptions)
				.OnGenerateWidget(this, &FDIVEActionBindingCustomization::MakePrimaryOptionWidget)
				.OnSelectionChanged(this, &FDIVEActionBindingCustomization::OnPrimaryOptionSelected)
				.InitiallySelectedItem(FindOptionByIndex(InitialPrimaryIndex))
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::Create(
						TAttribute<FText>::FGetter::CreateSP(
							this,
							&FDIVEActionBindingCustomization::GetPrimarySelectionLabel)))
				];
			PrimaryComboWeak = PrimaryCombo;

			ChildBuilder.AddCustomRow(PrimaryIndexHandle->GetPropertyDisplayName())
				.NameContent()
				[
					PrimaryIndexHandle->CreatePropertyNameWidget()
				]
				.ValueContent()
				.MinDesiredWidth(240.f)
				[
					PrimaryCombo
				];
		}
		else
		{
			ChildBuilder.AddProperty(ChildHandle.ToSharedRef());
		}

		if (ChildName == GET_MEMBER_NAME_CHECKED(FDIVEActionBinding, Targets))
		{
			ChildBuilder.AddCustomRow(LOCTEXT("TargetsPreviewFilter", "Targets"))
				.NameContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TargetsPreviewName", "Matched"))
					.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
				]
				.ValueContent()
				.MinDesiredWidth(280.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.f)
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(TAttribute<FText>::Create(
							TAttribute<FText>::FGetter::CreateSP(
								this,
								&FDIVEActionBindingCustomization::GetTargetsPreviewText)))
						.AutoWrapText(true)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(8.f, 0.f, 0.f, 0.f)
					.VAlign(VAlign_Center)
					[
						SNew(SButton)
						.Text(LOCTEXT("SelectMatching", "Select"))
						.ToolTipText(LOCTEXT(
							"SelectMatchingTooltip",
							"Select matching pickable components. Works on a placed device, or the Blueprint viewport preview."))
						.IsEnabled(TAttribute<bool>::Create(
							TAttribute<bool>::FGetter::CreateSP(
								this,
								&FDIVEActionBindingCustomization::CanSelectMatchingPrimitives)))
						.OnClicked(this, &FDIVEActionBindingCustomization::OnSelectMatchingPrimitives)
					]
				];
		}
	}
}

FText FDIVEActionBindingCustomization::GetCollapsedHeaderText() const
{
	if (!StructHandle.IsValid())
	{
		return LOCTEXT("BindingUntitled", "Binding");
	}

	TArray<void*> RawData;
	StructHandle->AccessRawData(RawData);
	if (RawData.Num() != 1 || !RawData[0])
	{
		return StructHandle->GetPropertyDisplayName();
	}

	const FDIVEActionBinding& Binding = *static_cast<const FDIVEActionBinding*>(RawData[0]);

	const FString IdLabel = Binding.BindingId.IsNone() ? FString() : Binding.BindingId.ToString();

	FString ActionLabel;
	if (const UDIVEDeviceAction* Primary = Binding.GetPrimaryAction())
	{
		ActionLabel = Primary->GetResolvedDisplayName().ToString();
	}

	if (!IdLabel.IsEmpty() && !ActionLabel.IsEmpty())
	{
		return FText::FromString(IdLabel + TEXT("  ·  ") + ActionLabel);
	}
	if (!IdLabel.IsEmpty())
	{
		return FText::FromString(IdLabel);
	}
	if (!ActionLabel.IsEmpty())
	{
		return FText::FromString(ActionLabel);
	}

	switch (Binding.Targets.MatchMode)
	{
	case EDIVETargetMatchMode::AnyPrimitive:
		return LOCTEXT("BindingAnyPrimitive", "Any Primitive");
	case EDIVETargetMatchMode::ComponentName:
		return Binding.Targets.MatchValues.Num() > 0
			? FText::Format(
				LOCTEXT("BindingName", "Name: {0}"),
				FText::FromName(Binding.Targets.MatchValues[0]))
			: LOCTEXT("BindingNameEmpty", "Component Name");
	case EDIVETargetMatchMode::PartId:
		return Binding.Targets.MatchValues.Num() > 0
			? FText::Format(
				LOCTEXT("BindingPartId", "PartId: {0}"),
				FText::FromName(Binding.Targets.MatchValues[0]))
			: LOCTEXT("BindingPartIdEmpty", "Part Id");
	case EDIVETargetMatchMode::ComponentTag:
	default:
		return Binding.Targets.MatchValues.Num() > 0
			? FText::Format(
				LOCTEXT("BindingTag", "Tag: {0}"),
				FText::FromName(Binding.Targets.MatchValues[0]))
			: LOCTEXT("BindingUntitled", "Binding");
	}
}

void FDIVEActionBindingCustomization::RebuildPrimaryOptions()
{
	PrimaryOptions.Reset();

	TSharedPtr<FPrimaryOption> NoneOption = MakeShared<FPrimaryOption>();
	NoneOption->Index = INDEX_NONE;
	NoneOption->Label = LOCTEXT("PrimaryNone", "None");
	PrimaryOptions.Add(NoneOption);

	if (ActionsHandle.IsValid())
	{
		uint32 NumActions = 0;
		ActionsHandle->GetNumChildren(NumActions);
		for (uint32 ActionIndex = 0; ActionIndex < NumActions; ++ActionIndex)
		{
			TSharedPtr<IPropertyHandle> ElementHandle = ActionsHandle->GetChildHandle(ActionIndex);
			UObject* ActionObject = nullptr;
			if (ElementHandle.IsValid())
			{
				if (ElementHandle->GetValue(ActionObject) != FPropertyAccess::Success)
				{
					TArray<void*> RawData;
					ElementHandle->AccessRawData(RawData);
					if (RawData.Num() > 0 && RawData[0])
					{
						ActionObject = static_cast<TObjectPtr<UObject>*>(RawData[0])->Get();
					}
				}
			}

			TSharedPtr<FPrimaryOption> Option = MakeShared<FPrimaryOption>();
			Option->Index = static_cast<int32>(ActionIndex);
			if (const UDIVEDeviceAction* Action = Cast<UDIVEDeviceAction>(ActionObject))
			{
				Option->Label = FText::Format(
					LOCTEXT("PrimaryNamed", "[{0}] {1}"),
					FText::AsNumber(Option->Index),
					Action->GetResolvedDisplayName());
			}
			else
			{
				Option->Label = FText::Format(
					LOCTEXT("PrimaryNull", "[{0}] (null)"),
					FText::AsNumber(Option->Index));
			}
			PrimaryOptions.Add(Option);
		}
	}

	if (TSharedPtr<SComboBox<TSharedPtr<FPrimaryOption>>> Combo = PrimaryComboWeak.Pin())
	{
		Combo->RefreshOptions();

		int32 Current = INDEX_NONE;
		if (PrimaryIndexHandle.IsValid())
		{
			PrimaryIndexHandle->GetValue(Current);
		}
		Combo->SetSelectedItem(FindOptionByIndex(Current));
	}
}

FText FDIVEActionBindingCustomization::GetPrimarySelectionLabel() const
{
	int32 Current = INDEX_NONE;
	if (PrimaryIndexHandle.IsValid())
	{
		PrimaryIndexHandle->GetValue(Current);
	}

	if (const TSharedPtr<FPrimaryOption> Option = FindOptionByIndex(Current))
	{
		return Option->Label;
	}

	return FText::Format(
		LOCTEXT("PrimaryOutOfRange", "[{0}] (out of range)"),
		FText::AsNumber(Current));
}

TSharedRef<SWidget> FDIVEActionBindingCustomization::MakePrimaryOptionWidget(
	TSharedPtr<FPrimaryOption> Option) const
{
	return SNew(STextBlock)
		.Text(Option.IsValid() ? Option->Label : LOCTEXT("PrimaryInvalid", "(invalid)"));
}

void FDIVEActionBindingCustomization::OnPrimaryOptionSelected(
	TSharedPtr<FPrimaryOption> Option,
	ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;
	if (!Option.IsValid() || !PrimaryIndexHandle.IsValid())
	{
		return;
	}

	PrimaryIndexHandle->SetValue(Option->Index);
}

TSharedPtr<FDIVEActionBindingCustomization::FPrimaryOption>
FDIVEActionBindingCustomization::FindOptionByIndex(const int32 Index) const
{
	for (const TSharedPtr<FPrimaryOption>& Option : PrimaryOptions)
	{
		if (Option.IsValid() && Option->Index == Index)
		{
			return Option;
		}
	}
	return nullptr;
}

const FDIVETargetQuery* FDIVEActionBindingCustomization::GetTargetQuery() const
{
	if (!TargetsHandle.IsValid())
	{
		return nullptr;
	}

	TArray<void*> RawData;
	TargetsHandle->AccessRawData(RawData);
	if (RawData.Num() != 1 || !RawData[0])
	{
		return nullptr;
	}

	return static_cast<const FDIVETargetQuery*>(RawData[0]);
}

UDIVEInspectableComponent* FDIVEActionBindingCustomization::ResolvePreviewInspectable() const
{
	if (!StructHandle.IsValid())
	{
		return nullptr;
	}

	TArray<UObject*> Outers;
	StructHandle->GetOuterObjects(Outers);
	for (UObject* Outer : Outers)
	{
		if (UDIVEInspectableComponent* Inspectable = Cast<UDIVEInspectableComponent>(Outer))
		{
			return Inspectable;
		}

		if (AActor* Actor = Cast<AActor>(Outer))
		{
			if (UDIVEInspectableComponent* Inspectable =
				Actor->FindComponentByClass<UDIVEInspectableComponent>())
			{
				return Inspectable;
			}
		}

		if (const UDIVEActionCatalogAsset* Catalog = Cast<UDIVEActionCatalogAsset>(Outer))
		{
			if (!GEditor)
			{
				continue;
			}

			USelection* SelectedActors = GEditor->GetSelectedActors();
			if (!SelectedActors)
			{
				continue;
			}

			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AActor* Actor = Cast<AActor>(*It);
				if (!Actor)
				{
					continue;
				}

				UDIVEInspectableComponent* Inspectable =
					Actor->FindComponentByClass<UDIVEInspectableComponent>();
				if (Inspectable && Inspectable->ActionCatalog == Catalog)
				{
					return Inspectable;
				}
			}
		}
	}

	return nullptr;
}

UDIVEInspectableComponent* FDIVEActionBindingCustomization::ResolveLiveInspectable() const
{
	UDIVEInspectableComponent* Inspectable = ResolvePreviewInspectable();
	if (!Inspectable)
	{
		return nullptr;
	}

	if (IsLiveWorldActor(Inspectable->GetOwner()))
	{
		return Inspectable;
	}

	UClass* ActorClass = nullptr;
	if (AActor* Owner = Inspectable->GetOwner())
	{
		ActorClass = Owner->GetClass();
	}
	else if (UClass* OuterClass = Inspectable->GetTypedOuter<UClass>())
	{
		ActorClass = OuterClass->IsChildOf(AActor::StaticClass()) ? OuterClass : nullptr;
	}

	if (!ActorClass || !ActorClass->IsChildOf(AActor::StaticClass()) || !GEngine)
	{
		return nullptr;
	}

	auto FindInWorldType = [&](const EWorldType::Type WorldType) -> UDIVEInspectableComponent*
	{
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || World->WorldType != WorldType)
			{
				continue;
			}

			for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
			{
				AActor* Actor = *It;
				if (!IsLiveWorldActor(Actor))
				{
					continue;
				}

				if (UDIVEInspectableComponent* Live = Actor->FindComponentByClass<UDIVEInspectableComponent>())
				{
					return Live;
				}
			}
		}

		return nullptr;
	};

	if (UDIVEInspectableComponent* Preview = FindInWorldType(EWorldType::EditorPreview))
	{
		return Preview;
	}

	if (GEditor)
	{
		if (USelection* SelectedActors = GEditor->GetSelectedActors())
		{
			for (FSelectionIterator It(*SelectedActors); It; ++It)
			{
				AActor* Actor = Cast<AActor>(*It);
				if (!Actor || !Actor->IsA(ActorClass) || !IsLiveWorldActor(Actor))
				{
					continue;
				}

				if (UDIVEInspectableComponent* Live = Actor->FindComponentByClass<UDIVEInspectableComponent>())
				{
					return Live;
				}
			}
		}
	}

	if (UDIVEInspectableComponent* Editor = FindInWorldType(EWorldType::Editor))
	{
		return Editor;
	}
	return FindInWorldType(EWorldType::PIE);
}

void FDIVEActionBindingCustomization::InvalidateMatchedPreview()
{
	MatchedPreview.bValid = false;
}

void FDIVEActionBindingCustomization::EnsureMatchedPreview() const
{
	const FDIVETargetQuery* Query = GetTargetQuery();
	UDIVEInspectableComponent* Inspectable = ResolveLiveInspectable();
	if (!Query || !Inspectable || !Inspectable->GetOwner())
	{
		MatchedPreview.bValid = false;
		MatchedPreview.Primitives.Reset();
		return;
	}

	if (MatchedPreview.bValid
		&& MatchedPreview.Inspectable.Get() == Inspectable
		&& MatchedPreview.MatchMode == Query->MatchMode
		&& MatchedPreview.MatchValues == Query->MatchValues)
	{
		bool bAllLive = true;
		for (const TWeakObjectPtr<UPrimitiveComponent>& PrimitiveWeak : MatchedPreview.Primitives)
		{
			if (!PrimitiveWeak.IsValid())
			{
				bAllLive = false;
				break;
			}
		}
		if (bAllLive)
		{
			return;
		}
	}

	TArray<UPrimitiveComponent*> Matching;
	Inspectable->CollectPrimitivesMatchingQuery(*Query, Matching);

	MatchedPreview.Inspectable = Inspectable;
	MatchedPreview.MatchMode = Query->MatchMode;
	MatchedPreview.MatchValues = Query->MatchValues;
	MatchedPreview.Primitives.Reset();
	MatchedPreview.Primitives.Reserve(Matching.Num());
	for (UPrimitiveComponent* Primitive : Matching)
	{
		MatchedPreview.Primitives.Add(Primitive);
	}
	MatchedPreview.bValid = true;
}

void FDIVEActionBindingCustomization::CollectMatchingPrimitives(
	TArray<UPrimitiveComponent*>& OutPrimitives) const
{
	OutPrimitives.Reset();
	EnsureMatchedPreview();
	for (const TWeakObjectPtr<UPrimitiveComponent>& PrimitiveWeak : MatchedPreview.Primitives)
	{
		if (UPrimitiveComponent* Primitive = PrimitiveWeak.Get())
		{
			OutPrimitives.Add(Primitive);
		}
	}
}

FText FDIVEActionBindingCustomization::GetTargetsPreviewText() const
{
	if (!StructHandle.IsValid())
	{
		return LOCTEXT("TargetsUnavailable", "Targets: —");
	}

	TArray<UObject*> Outers;
	StructHandle->GetOuterObjects(Outers);
	if (Outers.Num() > 1)
	{
		return LOCTEXT("TargetsMultiple", "Targets: — (multiple values)");
	}

	bool bEditingCatalog = false;
	for (UObject* Outer : Outers)
	{
		if (Cast<UDIVEActionCatalogAsset>(Outer))
		{
			bEditingCatalog = true;
			break;
		}
	}

	UDIVEInspectableComponent* Inspectable = ResolveLiveInspectable();
	if (!Inspectable || !Inspectable->GetOwner())
	{
		if (bEditingCatalog)
		{
			return LOCTEXT(
				"TargetsSelectCatalogDevice",
				"Targets: — (select a device using this catalog)");
		}

		return LOCTEXT(
			"TargetsNeedLiveDevice",
			"Targets: — (open the Blueprint viewport or select a placed device)");
	}

	const FDIVETargetQuery* Query = GetTargetQuery();
	if (!Query)
	{
		return LOCTEXT("TargetsUnavailable", "Targets: —");
	}

	TArray<UPrimitiveComponent*> Matching;
	CollectMatchingPrimitives(Matching);

	if (Query->MatchMode == EDIVETargetMatchMode::AnyPrimitive)
	{
		return FText::Format(
			LOCTEXT("TargetsAny", "Targets: {0} components (any pickable)"),
			FText::AsNumber(Matching.Num()));
	}

	return FText::Format(
		LOCTEXT("TargetsCount", "Targets: {0} components"),
		FText::AsNumber(Matching.Num()));
}

bool FDIVEActionBindingCustomization::CanSelectMatchingPrimitives() const
{
	TArray<UPrimitiveComponent*> Matching;
	CollectMatchingPrimitives(Matching);
	return Matching.Num() > 0;
}

FReply FDIVEActionBindingCustomization::OnSelectMatchingPrimitives()
{
	TArray<UPrimitiveComponent*> Matching;
	CollectMatchingPrimitives(Matching);
	if (Matching.IsEmpty())
	{
		return FReply::Handled();
	}

	const UWorld* World = Matching[0] ? Matching[0]->GetWorld() : nullptr;
	if (IsLevelSelectableWorld(World) && GEditor)
	{
		TSet<AActor*> Owners;
		for (UPrimitiveComponent* Primitive : Matching)
		{
			if (Primitive && Primitive->GetOwner())
			{
				Owners.Add(Primitive->GetOwner());
			}
		}

		GEditor->SelectNone(/*bNoteSelectionChange=*/ false, /*bDeselectBSPSurfs=*/ true, /*WarnAboutManyActors=*/ false);
		for (AActor* Actor : Owners)
		{
			GEditor->SelectActor(Actor, /*bInSelected=*/ true, /*bNotify=*/ false, /*bSelectEvenIfHidden=*/ true);
		}
		for (UPrimitiveComponent* Primitive : Matching)
		{
			if (Primitive)
			{
				GEditor->SelectComponent(Primitive, /*bInSelected=*/ true, /*bNotify=*/ false, /*bSelectEvenIfHidden=*/ true);
			}
		}
		GEditor->NoteSelectionChange();
		return FReply::Handled();
	}

	TrySelectMatchingInBlueprintEditor(Matching);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
