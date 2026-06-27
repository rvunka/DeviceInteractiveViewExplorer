// Copyright (c) 2026. All Rights Reserved.

#include "DIVEAnchorComponent.h"

#include "DIVEConvention.h"
#include "Components/ArrowComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"

UDIVEAnchorComponent::UDIVEAnchorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDIVEAnchorComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	if (bShowViewDirection && GetWorld() && !GetWorld()->IsGameWorld())
	{
		EnsureEditorViewDirectionArrow();
		RefreshViewDirectionArrow();
	}
#endif
}

bool UDIVEAnchorComponent::CanOwnRuntimeVisuals() const
{
	const AActor* Owner = GetOwner();
	return Owner && !Owner->HasAnyFlags(RF_ClassDefaultObject) && !IsTemplate();
}

void UDIVEAnchorComponent::EnsureEditorViewDirectionArrow()
{
#if WITH_EDITOR
	if (ViewDirectionArrow && ViewDirectionArrow->IsRegistered())
	{
		return;
	}

	if (!CanOwnRuntimeVisuals() || !bShowViewDirection)
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
	ViewDirectionArrow->SetArrowColor(FLinearColor(0.1f, 0.85f, 1.f));
	ViewDirectionArrow->RegisterComponent();
	RefreshViewDirectionArrow();
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

void UDIVEAnchorComponent::EnsureSessionMarkerMesh()
{
	if (SessionMarkerMesh && SessionMarkerMesh->IsRegistered())
	{
		return;
	}

	if (!CanOwnRuntimeVisuals())
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	SessionMarkerMesh = NewObject<UStaticMeshComponent>(
		Owner,
		UStaticMeshComponent::StaticClass(),
		MakeUniqueObjectName(Owner, UStaticMeshComponent::StaticClass(), TEXT("DIVE_SessionMarker")),
		RF_Transient);
	SessionMarkerMesh->SetupAttachment(this);
	SessionMarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SessionMarkerMesh->SetGenerateOverlapEvents(false);
	SessionMarkerMesh->SetCastShadow(false);
	SessionMarkerMesh->SetHiddenInGame(true);
	SessionMarkerMesh->ComponentTags.Add(DIVE::kAnchorMarkerTag);
	SessionMarkerMesh->RegisterComponent();
	ConfigureSessionMarker();
}

void UDIVEAnchorComponent::DestroyTransientVisuals()
{
	if (SessionMarkerMesh)
	{
		SessionMarkerMesh->DestroyComponent();
		SessionMarkerMesh = nullptr;
	}

	if (ViewDirectionArrow)
	{
		ViewDirectionArrow->DestroyComponent();
		ViewDirectionArrow = nullptr;
	}
}

void UDIVEAnchorComponent::OnUnregister()
{
	DestroyTransientVisuals();
	Super::OnUnregister();
}

void UDIVEAnchorComponent::ConfigureSessionMarker()
{
	if (!SessionMarkerMesh)
	{
		return;
	}

	if (UStaticMesh* SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")))
	{
		SessionMarkerMesh->SetStaticMesh(SphereMesh);
	}

	const float ClampedScale = FMath::Max(MarkerScale, 0.01f);
	SessionMarkerMesh->SetRelativeScale3D(FVector(ClampedScale));
}

FRotator UDIVEAnchorComponent::GetViewRotation() const
{
	return bUseComponentRotationForView ? GetComponentRotation() : ViewRotationOverride;
}

FName UDIVEAnchorComponent::GetResolvedPartId() const
{
	return PartId.IsNone() ? GetFName() : PartId;
}

void UDIVEAnchorComponent::SetSessionPresentation(bool bSessionActive, bool bHidePickMarker)
{
	if (bSessionActive && bShowSessionMarker)
	{
		EnsureSessionMarkerMesh();
	}

	const bool bShowMarker = bSessionActive && bShowSessionMarker && !bHidePickMarker;
	if (SessionMarkerMesh && bShowSessionMarker)
	{
		SessionMarkerMesh->SetHiddenInGame(!bShowMarker);
		SessionMarkerMesh->SetVisibility(bShowMarker, true);

		if (bShowMarker)
		{
			ConfigureSessionMarker();
		}
	}
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

	if (SessionMarkerMesh)
	{
		ConfigureSessionMarker();
	}

	RefreshViewDirectionArrow();
}
#endif
