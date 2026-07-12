// Copyright (c) 2026. All Rights Reserved.

#include "DIVEAnchorComponent.h"

#include "Components/ArrowComponent.h"
#include "Engine/World.h"

namespace
{
bool CanRegisterEditorSubcomponent()
{
#if WITH_EDITOR
	if (IsRunningCommandlet() || GIsAutomationTesting)
	{
		return false;
	}

	return true;
#else
	return false;
#endif
}
}

UDIVEAnchorComponent::UDIVEAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDIVEAnchorComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	if (!bShowViewDirection || !CanOwnEditorVisuals())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->IsGameWorld() || !CanRegisterEditorSubcomponent())
	{
		return;
	}

	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(
		this,
		[this]()
		{
			if (!IsValid(this) || !bShowViewDirection)
			{
				return;
			}

			EnsureEditorViewDirectionArrow();
			RefreshViewDirectionArrow();
		}));
#endif
}

bool UDIVEAnchorComponent::CanOwnEditorVisuals() const
{
	const AActor* Owner = GetOwner();
	return Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject) && !IsTemplate();
}

void UDIVEAnchorComponent::EnsureEditorViewDirectionArrow()
{
#if WITH_EDITOR
	if (!CanRegisterEditorSubcomponent())
	{
		return;
	}

	if (ViewDirectionArrow && ViewDirectionArrow->IsRegistered())
	{
		return;
	}

	if (!CanOwnEditorVisuals() || !bShowViewDirection)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	ViewDirectionArrow = NewObject<UArrowComponent>(
		Owner,
		UArrowComponent::StaticClass(),
		MakeUniqueObjectName(Owner, UArrowComponent::StaticClass(), TEXT("DIVE_ViewDirection")),
		RF_Transient);
	ViewDirectionArrow->SetupAttachment(this);
	ViewDirectionArrow->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ViewDirectionArrow->SetHiddenInGame(true);
	ViewDirectionArrow->RegisterComponent();
#endif
}

void UDIVEAnchorComponent::RefreshViewDirectionArrow()
{
	if (!ViewDirectionArrow)
	{
		return;
	}

	ViewDirectionArrow->SetWorldRotation(GetViewRotation());
	ViewDirectionArrow->SetArrowSize(FMath::Max(ViewDirectionScale, 0.05f));
}

void UDIVEAnchorComponent::DestroyEditorVisuals()
{
	if (ViewDirectionArrow)
	{
		ViewDirectionArrow->DestroyComponent();
		ViewDirectionArrow = nullptr;
	}
}

void UDIVEAnchorComponent::OnUnregister()
{
	DestroyEditorVisuals();
	Super::OnUnregister();
}

FRotator UDIVEAnchorComponent::GetViewRotation() const
{
	return bUseComponentRotationForView ? GetComponentRotation() : ViewRotationOverride;
}

FName UDIVEAnchorComponent::GetResolvedPartId() const
{
	return PartId.IsNone() ? GetFName() : PartId;
}

#if WITH_EDITOR
void UDIVEAnchorComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UDIVEAnchorComponent, bShowViewDirection))
	{
		if (bShowViewDirection)
		{
			EnsureEditorViewDirectionArrow();
		}
		else if (ViewDirectionArrow)
		{
			ViewDirectionArrow->DestroyComponent();
			ViewDirectionArrow = nullptr;
		}
	}

	RefreshViewDirectionArrow();
}
#endif
