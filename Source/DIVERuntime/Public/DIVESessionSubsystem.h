// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DIVEDeviceAction.h"
#include "DIVEPawnPhysicalDrive.h"
#include "DIVETypes.h"
#include "Engine/HitResult.h"
#include "DIVESessionSubsystem.generated.h"

class ADIVECameraRig;
class UDIVEInspectableComponent;
class UDIVEProxyDriveForwardAction;
class UMaterialInterface;
class UPrimitiveComponent;
class AActor;
class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDIVESessionStarted, AActor*, DeviceHost, UDIVEInspectableComponent*, Inspectable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDIVESessionEnded, EDIVESessionEndReason, Reason, AActor*, DeviceHost);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVEFocusChanged, const FDIVEFocusTarget&, FocusTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVEContextMenuVisibilityChanged, bool, bIsOpen);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDIVEInteractionModeChanged, EDIVESessionInteractionMode, NewMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDIVEInteractionValueChanged,
	UDIVEDeviceAction*,
	Action,
	const FDIVEActionContext&,
	Context,
	const FDIVEInteractionValue&,
	Value);

struct FDIVESessionFocusOps;
struct FDIVESessionIsolationOps;
struct FDIVESessionPhysicalDriveOps;
struct FDIVESessionPickOps;

UCLASS()
class DIVERUNTIME_API UDIVESessionSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsSessionActive() const { return SessionState == EDIVESessionState::Active; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	AActor* GetActiveDeviceHost() const { return ActiveDeviceHost.Get(); }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	UDIVEInspectableComponent* GetActiveInspectable() const { return ActiveInspectable.Get(); }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	ADIVECameraRig* GetActiveCameraRig() const { return ActiveCameraRig.Get(); }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	FDIVEFocusTarget GetFocusedTarget() const { return FocusedTarget; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsIsolationActive() const { return bIsolationActive; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	FDIVEFocusTarget GetIsolationTarget() const { return IsolationTarget; }

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool IsIsolationActiveForTarget(const FDIVEFocusTarget& Target) const;

	/** True while device proxy drive or pawn physical drive is active. */
	UFUNCTION(BlueprintPure, Category = "DIVE|ProxyDrive")
	bool IsProxyDriving() const { return bProxyDriving; }

	/** Call from PrimaryActionReleased after menu-started continuous Begin. */
	bool ConsumeIgnoreNextPrimaryActionRelease();

	UFUNCTION(BlueprintPure, Category = "DIVE|ProxyDrive")
	bool IsPawnPhysicalDriveActive() const;

	UFUNCTION(BlueprintPure, Category = "DIVE|Session")
	EDIVESessionInteractionMode GetInteractionMode() const { return InteractionMode; }

	UFUNCTION(BlueprintCallable, Category = "DIVE|Session")
	void SetInteractionMode(EDIVESessionInteractionMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool TryBeginSession(AActor* DeviceHost, UDIVEInspectableComponent* Inspectable, const FDIVESessionParams& Params);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void EndSession(EDIVESessionEndReason Reason = EDIVESessionEndReason::UserExit);

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Session")
	FOnDIVESessionStarted OnSessionStarted;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Session")
	FOnDIVESessionEnded OnSessionEnded;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Session")
	FOnDIVEFocusChanged OnFocusChanged;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|ContextMenu")
	FOnDIVEContextMenuVisibilityChanged OnContextMenuVisibilityChanged;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Session")
	FOnDIVEInteractionModeChanged OnInteractionModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "DIVE|Action")
	FOnDIVEInteractionValueChanged OnInteractionValueChanged;

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyOrbitInput(const FVector2D& Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyZoomInput(float Delta);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ConfigureActiveCameraInput(float OrbitSensitivity, float ZoomSensitivity);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ApplyCameraInputFromInspectable();

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool FocusAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Pick")
	bool ResolvePickAtScreenPosition(
		const FVector2D& ScreenPosition,
		APlayerController* PlayerController,
		FDIVEFocusTarget& OutPickTarget) const;

	UFUNCTION(BlueprintCallable, Category = "DIVE|Pick")
	bool ExecutePrimaryActionAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Pick")
	void UpdatePickHover(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE|Pick")
	void ClearPickHover();

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool FocusTarget(const FDIVEFocusTarget& Target, bool bPushToStack = true);

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool NavigateBack();

	UFUNCTION(BlueprintPure, Category = "DIVE")
	bool CanNavigateBack() const { return IsSessionActive() && FocusStack.Num() > 1; }

	UFUNCTION(BlueprintPure, Category = "DIVE|ContextMenu")
	bool IsContextMenuOpen() const { return bContextMenuOpen; }

	/** Opens the menu at the screen position. Returns true if the menu is open after the call (false if toggled closed or build failed). */
	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu")
	bool OpenContextMenuAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu")
	bool ExecuteContextMenuAction(
		UDIVEDeviceAction* Action,
		FName TargetKey = NAME_None,
		FName BindingId = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu")
	void CloseContextMenu();

	const TArray<FDIVEContextMenuEntry>& GetContextMenuEntries() const { return ContextMenuEntries; }

	FVector2D GetContextMenuScreenPosition() const { return ContextMenuScreenPosition; }

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool ToggleIsolateFocused();

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	bool ToggleIsolationForTarget(const FDIVEFocusTarget& Target);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu|Admin")
	bool ToggleMeshPhysicsForTarget(const FDIVEFocusTarget& Target);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ContextMenu|Admin")
	bool DeleteMeshForTarget(const FDIVEFocusTarget& Target);

	/** Non-Shipping only. */
	static bool AreAdminContextMenuEntriesAllowed();

	UFUNCTION(BlueprintCallable, Category = "DIVE")
	void ClearIsolation();

	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool TryBeginProxyDriveAtScreenPosition(const FVector2D& ScreenPosition, APlayerController* PlayerController);

	/** Updates active proxy drive or continuous device action. */
	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	void UpdateActiveInteraction(const FDIVEInteractionUpdate& Update);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	void EndProxyDrive(bool bCommit);

	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	void HandleActivePawnPhysicalManualRotatePressed();

	UFUNCTION(BlueprintCallable, Category = "DIVE|ProxyDrive")
	void HandleActivePawnPhysicalManualRotateReleased();

	bool TryBeginContinuousAction(UDIVEContinuousDeviceAction* Action, const FDIVEActionContext& Context);

	void NotifyInteractionValueChanged(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		const FDIVEInteractionValue& Value);
	void HandleWorldBeginTearDown(UWorld* InWorld);

	UFUNCTION()
	void HandleContinuousActionValueChanged(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		const FDIVEInteractionValue& Value);

	friend struct FDIVESessionFocusOps;
	friend struct FDIVESessionIsolationOps;
	friend struct FDIVESessionPhysicalDriveOps;
	friend struct FDIVESessionPickOps;

private:
	enum class EDIVEActivePhysicalDriveKind : uint8
	{
		None,
		PawnDrive,
		ContinuousAction
	};

	EDIVESessionState SessionState = EDIVESessionState::Inactive;

	TWeakObjectPtr<AActor> ActiveDeviceHost;
	TWeakObjectPtr<UDIVEInspectableComponent> ActiveInspectable;
	TWeakObjectPtr<ADIVECameraRig> ActiveCameraRig;
	TWeakObjectPtr<AActor> PreviousViewTarget;

	FDIVEFocusTarget FocusedTarget = FDIVEFocusTarget::MakeDeviceRoot();
	TArray<FDIVEFocusTarget> FocusStack;

	bool bIsolationActive = false;
	FDIVEFocusTarget IsolationTarget = FDIVEFocusTarget::MakeDeviceRoot();

	struct FIsolatedPrimitiveRecord
	{
		TWeakObjectPtr<UPrimitiveComponent> Primitive;
		bool bWasHiddenInGame = false;
	};
	TArray<FIsolatedPrimitiveRecord> IsolatedHiddenPrimitives;

	bool bProxyDriving = false;
	EDIVEActivePhysicalDriveKind ActivePhysicalDriveKind = EDIVEActivePhysicalDriveKind::None;
	TWeakInterfacePtr<IDIVEPawnPhysicalDrive> ActivePawnPhysicalDrive;
	TWeakObjectPtr<UDIVEContinuousDeviceAction> ActiveContinuousAction;

	/**
	 * Internal action instance that routes Physical-mode IDIVEProxyDrive hits through the standard
	 * continuous-action slot (pre-rev2). Target: Interact catalog / ForwardAction, not Physical pick.
	 * Created once per session in TryBeginSession and reused.
	 */
	UPROPERTY(Transient)
	TObjectPtr<UDIVEProxyDriveForwardAction> InternalProxyDriveAction;

	EDIVESessionInteractionMode InteractionMode = EDIVESessionInteractionMode::Interact;

	float SessionDefaultOrbitDistance = DIVE::kDefaultOrbitDistance;

	bool bContextMenuOpen = false;
	FDIVEFocusTarget ContextMenuPickTarget;
	TArray<FDIVEContextMenuEntry> ContextMenuEntries;
	FVector2D ContextMenuScreenPosition = FVector2D::ZeroVector;
	FHitResult ContextMenuPickHit;
	bool bIgnoreNextPrimaryActionRelease = false;

	TWeakObjectPtr<UPrimitiveComponent> PickHoverPrimitive;
	/** The overlay material that was on PickHoverPrimitive before DIVE applied the hover highlight. */
	TWeakObjectPtr<UMaterialInterface> PickHoverPreviousOverlay;

	FVector2D LastPickHoverScreenPosition = FVector2D::ZeroVector;
	bool bHasLastPickHoverScreenPosition = false;

	bool ResolvePickAtScreenPositionWithHit(
		const FVector2D& ScreenPosition,
		APlayerController* PlayerController,
		FDIVEFocusTarget& OutPickTarget,
		FHitResult& OutHit) const;

	bool BuildContextMenuEntries(
		const FVector2D& ScreenPosition,
		APlayerController* PlayerController,
		TArray<FDIVEContextMenuEntry>& OutEntries,
		FDIVEFocusTarget& OutPickTarget,
		FHitResult& OutPickHit) const;

	bool ExecuteResolvedAction(
		UDIVEDeviceAction* Action,
		const FDIVEActionContext& Context,
		bool bSetIgnoreNextReleaseIfContinuous);
};
