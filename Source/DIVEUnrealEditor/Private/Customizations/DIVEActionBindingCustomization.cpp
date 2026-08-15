// Copyright (c) 2026. All Rights Reserved.

#include "Customizations/DIVEActionBindingCustomization.h"

#include "DIVEActionBinding.h"
#include "DIVEActionCatalogAsset.h"
#include "DIVEDeviceAction.h"
#include "DIVEInspectableComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Containers/Set.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "IDetailChildrenBuilder.h"
#include "Input/Reply.h"
#include "PropertyHandle.h"
#include "Selection.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DIVEActionBindingCustomization"

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

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
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
							"Select matching pickable components on the device in the viewport."))
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

void FDIVEActionBindingCustomization::InvalidateMatchedPreview()
{
	MatchedPreview.bValid = false;
}

void FDIVEActionBindingCustomization::EnsureMatchedPreview() const
{
	const FDIVETargetQuery* Query = GetTargetQuery();
	UDIVEInspectableComponent* Inspectable = ResolvePreviewInspectable();
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
		return;
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

	UDIVEInspectableComponent* Inspectable = ResolvePreviewInspectable();
	if (!Inspectable || !Inspectable->GetOwner())
	{
		if (bEditingCatalog)
		{
			return LOCTEXT(
				"TargetsSelectCatalogDevice",
				"Targets: — (select a device using this catalog)");
		}

		return LOCTEXT(
			"TargetsPlaceDevice",
			"Targets: — (place or select the device)");
	}

	const FDIVETargetQuery* Query = GetTargetQuery();
	if (!Query)
	{
		return LOCTEXT("TargetsUnavailable", "Targets: —");
	}

	EnsureMatchedPreview();

	if (Query->MatchMode == EDIVETargetMatchMode::AnyPrimitive)
	{
		return FText::Format(
			LOCTEXT("TargetsAny", "Targets: {0} components (any pickable)"),
			FText::AsNumber(MatchedPreview.Primitives.Num()));
	}

	return FText::Format(
		LOCTEXT("TargetsCount", "Targets: {0} components"),
		FText::AsNumber(MatchedPreview.Primitives.Num()));
}

bool FDIVEActionBindingCustomization::CanSelectMatchingPrimitives() const
{
	UDIVEInspectableComponent* Inspectable = ResolvePreviewInspectable();
	if (!Inspectable)
	{
		return false;
	}

	AActor* Owner = Inspectable->GetOwner();
	if (!Owner || Owner->HasAnyFlags(RF_ClassDefaultObject))
	{
		return false;
	}

	const UWorld* World = Owner->GetWorld();
	if (!World
		|| (World->WorldType != EWorldType::Editor && World->WorldType != EWorldType::PIE))
	{
		return false;
	}

	TArray<UPrimitiveComponent*> Matching;
	CollectMatchingPrimitives(Matching);
	return Matching.Num() > 0;
}

FReply FDIVEActionBindingCustomization::OnSelectMatchingPrimitives()
{
	if (!GEditor || !CanSelectMatchingPrimitives())
	{
		return FReply::Handled();
	}

	TArray<UPrimitiveComponent*> Matching;
	CollectMatchingPrimitives(Matching);
	if (Matching.IsEmpty())
	{
		return FReply::Handled();
	}

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

#undef LOCTEXT_NAMESPACE
