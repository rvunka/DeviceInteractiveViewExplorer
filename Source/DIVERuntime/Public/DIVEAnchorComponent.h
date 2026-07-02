// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "DIVEAnchorComponent.generated.h"

class UArrowComponent;
class UStaticMeshComponent;
class UDIVEInspectableComponent;

UCLASS(ClassGroup = (DIVE), meta = (BlueprintSpawnableComponent, DisplayName = "DIVE Anchor"))
class DIVERUNTIME_API UDIVEAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UDIVEAnchorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FName PartId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker")
	bool bShowSessionMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker", meta = (ClampMin = "0.01", EditCondition = "bShowSessionMarker"))
	float MarkerScale = 0.12f;

	/** Overrides inspectable Default Anchor Marker Mesh when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker", meta = (
		DisplayName = "Marker Mesh Override",
		EditCondition = "bShowSessionMarker",
		ToolTip = "Leave empty to use DIVE Inspectable → Default Anchor Marker Mesh."))
	TSoftObjectPtr<UStaticMesh> MarkerMeshOverride;

	/** Overrides inspectable Default Anchor Marker Material when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker", meta = (
		DisplayName = "Marker Material Override",
		EditCondition = "bShowSessionMarker",
		ToolTip = "Leave empty to use DIVE Inspectable → Default Anchor Marker Material. Color and opacity come from the assigned material."))
	TSoftObjectPtr<UMaterialInterface> MarkerMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker")
	bool bShowViewDirection = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker", meta = (ClampMin = "0.05", EditCondition = "bShowViewDirection", ToolTip = "Editor-only gizmo size. Hidden during PIE/gameplay."))
	float ViewDirectionScale = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|View")
	bool bUseComponentRotationForView = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|View", meta = (EditCondition = "!bUseComponentRotationForView"))
	FRotator ViewRotationOverride = FRotator(-20.f, 0.f, 0.f);

	UFUNCTION(BlueprintPure, Category = "DIVE|View")
	FRotator GetViewRotation() const;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	FName GetResolvedPartId() const;

	void SetSessionPresentation(bool bSessionActive, bool bHidePickMarker);

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> SessionMarkerMesh;

	UPROPERTY(Transient)
	TObjectPtr<UArrowComponent> ViewDirectionArrow;

	bool CanOwnRuntimeVisuals() const;
	void EnsureEditorViewDirectionArrow();
	void EnsureSessionMarkerMesh();
	void DestroyTransientVisuals();
	void RefreshViewDirectionArrow();
	void ConfigureSessionMarker();
	void ApplyMarkerCollision(UDIVEInspectableComponent* Inspectable);
};
