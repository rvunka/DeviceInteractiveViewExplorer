// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "DIVETypes.h"
#include "DIVEAnchorComponent.generated.h"

class UArrowComponent;
class UStaticMeshComponent;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE")
	TArray<FName> OperationIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Manipulator")
	EDIVEManipulationKind ManipulationKind = EDIVEManipulationKind::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Manipulator", meta = (EditCondition = "ManipulationKind == EDIVEManipulationKind::Hinge", EditConditionHides))
	FVector HingeAxisLocal = FVector(0.f, 1.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Manipulator", meta = (EditCondition = "ManipulationKind == EDIVEManipulationKind::Hinge", EditConditionHides))
	float HingeMinAngle = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Manipulator", meta = (EditCondition = "ManipulationKind == EDIVEManipulationKind::Hinge", EditConditionHides))
	float HingeMaxAngle = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Manipulator", meta = (EditCondition = "ManipulationKind == EDIVEManipulationKind::Hinge", EditConditionHides))
	TArray<float> HingeSnapAngles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Manipulator", meta = (ClampMin = "0.01", EditCondition = "ManipulationKind == EDIVEManipulationKind::Hinge", EditConditionHides))
	float HingeDragSensitivity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker")
	bool bShowSessionMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "DIVE|Marker", meta = (ClampMin = "0.01", EditCondition = "bShowSessionMarker"))
	float MarkerScale = 0.12f;

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

	UStaticMeshComponent* GetSessionMarkerMesh() const { return SessionMarkerMesh; }

	UFUNCTION(BlueprintPure, Category = "DIVE|Manipulator")
	float GetHingeAngleDegrees() const { return CurrentHingeAngleDegrees; }

	UFUNCTION(BlueprintPure, Category = "DIVE|Manipulator")
	bool SupportsManipulation() const { return ManipulationKind == EDIVEManipulationKind::Hinge; }

	void CaptureManipulationBase();
	void SetHingeAngleDegrees(float AngleDegrees);
	USceneComponent* GetManipulatedComponent() const;

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

	FRotator ManipulationBaseRotation = FRotator::ZeroRotator;
	float CurrentHingeAngleDegrees = 0.f;

	bool CanOwnRuntimeVisuals() const;
	void EnsureEditorViewDirectionArrow();
	void EnsureSessionMarkerMesh();
	void DestroyTransientVisuals();
	void RefreshViewDirectionArrow();
	void ConfigureSessionMarker();
};
