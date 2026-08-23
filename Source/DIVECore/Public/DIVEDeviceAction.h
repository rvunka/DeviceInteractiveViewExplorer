// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVETypes.h"
#include "Engine/HitResult.h"
#include "UObject/Object.h"

#include "DIVEDeviceAction.generated.h"

class AActor;
class UActorComponent;
class UPrimitiveComponent;
class UDIVEActionCondition;
class UDIVEDeviceAction;

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEActionContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	TObjectPtr<AActor> DeviceHost = nullptr;

	/** Source component (Inspectable); typed weakly so Core stays Runtime-free. */
	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	TObjectPtr<UActorComponent> SourceComponent = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	TObjectPtr<UPrimitiveComponent> Target = nullptr;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FName TargetKey = NAME_None;

	/** Slot that produced this invocation. None if the action was not resolved from a binding. */
	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FName BindingId = NAME_None;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FDIVEFocusTarget PickTarget;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FHitResult PickHit;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector ViewLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FRotator ViewRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector PickRayDir = FVector::ForwardVector;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEActionDisplayState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FText DisplayName;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	bool bEnabled = true;

	/** false = omit from menu (not merely disable). */
	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	bool bVisible = true;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	bool bChecked = false;
};

UENUM(BlueprintType)
enum class EDIVEInteractionValueUnit : uint8
{
	None UMETA(DisplayName = "None"),
	Degrees UMETA(DisplayName = "Degrees"),
	Turns UMETA(DisplayName = "Turns"),
	Centimeters UMETA(DisplayName = "Centimeters")
};

/** Numeric payload for a continuous gesture. Normalized is always 0..1; Absolute is the domain number. */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEInteractionValue
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	float Normalized = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	float Absolute = 0.f;

	/** 0 = readout omits a max (e.g. unlimited rotary). */
	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	float AbsoluteMax = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	EDIVEInteractionValueUnit Unit = EDIVEInteractionValueUnit::None;
};

/** Per-frame payload for continuous hold/drag updates (cursor, ray, view). */
USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEInteractionUpdate
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector2D ScreenDelta = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	float DeltaTime = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector ViewLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FRotator ViewRotation = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadWrite, Category = "DIVE")
	FVector PickRayDir = FVector::ForwardVector;
};

namespace DIVE
{
	DIVECORE_API FText FormatInteractionValueReadout(const FText& Label, const FDIVEInteractionValue& Value);
}

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnDIVEActionExecuted,
	UDIVEDeviceAction*,
	Action,
	const FDIVEActionContext&,
	Context);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnDIVEActionValueChanged,
	UDIVEDeviceAction*,
	Action,
	const FDIVEActionContext&,
	Context,
	const FDIVEInteractionValue&,
	Value);

UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced,
	meta = (DisplayName = "DIVE Action Condition"))
class DIVECORE_API UDIVEActionCondition : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Condition")
	bool Evaluate(const FDIVEActionContext& Context) const;
	virtual bool Evaluate_Implementation(const FDIVEActionContext& Context) const;
};

/** Instant action. Params on the instance; per-target runtime state must not live on instance fields. */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class DIVECORE_API UDIVEDeviceAction : public UObject
{
	GENERATED_BODY()

public:
	/** Empty = class display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FText DisplayName;

	UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Action", meta = (
		ToolTip = "Optional. None = always show. DIVE Action Condition BP for Evaluate."))
	TObjectPtr<UDIVEActionCondition> Condition;

	/** Fan-out from UDIVEInspectableComponent::NotifyActionExecuted after a successful session Execute/Begin. */
	UPROPERTY(BlueprintAssignable, Category = "Action")
	FOnDIVEActionExecuted OnExecuted;

	virtual UWorld* GetWorld() const override;

	/** Injected by FDIVEActionWorldScope so catalog-hosted actions have a world during invocation. */
	void SetExecutionWorld(UWorld* InWorld) { ExecutionWorld = InWorld; }
	UWorld* GetExecutionWorld() const { return ExecutionWorld.Get(); }

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action")
	bool CanExecute(const FDIVEActionContext& Context) const;
	virtual bool CanExecute_Implementation(const FDIVEActionContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action")
	FDIVEActionDisplayState GetDisplayState(const FDIVEActionContext& Context) const;
	virtual FDIVEActionDisplayState GetDisplayState_Implementation(const FDIVEActionContext& Context) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action")
	bool Execute(const FDIVEActionContext& Context);
	virtual bool Execute_Implementation(const FDIVEActionContext& Context);

	FText GetResolvedDisplayName() const;

private:
	TWeakObjectPtr<UWorld> ExecutionWorld;
};

/** RAII: binds Action's execution world for the duration of the scope, then restores the previous world. */
struct DIVECORE_API FDIVEActionWorldScope
{
	FDIVEActionWorldScope(UDIVEDeviceAction* InAction, UWorld* InWorld)
		: Action(InAction)
	{
		if (Action)
		{
			PreviousWorld = Action->GetExecutionWorld();
			Action->SetExecutionWorld(InWorld);
		}
	}

	~FDIVEActionWorldScope()
	{
		if (Action)
		{
			Action->SetExecutionWorld(PreviousWorld.Get());
		}
	}

	FDIVEActionWorldScope(const FDIVEActionWorldScope&) = delete;
	FDIVEActionWorldScope& operator=(const FDIVEActionWorldScope&) = delete;

private:
	UDIVEDeviceAction* Action = nullptr;
	TWeakObjectPtr<UWorld> PreviousWorld;
};

/** Hold/drag action (Begin / Update / End). */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class DIVECORE_API UDIVEContinuousDeviceAction : public UDIVEDeviceAction
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Action")
	FOnDIVEActionValueChanged OnValueChanged;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action")
	bool BeginInteraction(const FDIVEActionContext& Context);
	virtual bool BeginInteraction_Implementation(const FDIVEActionContext& Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action", meta = (
		ToolTip = "v0.8-dev: receives FDIVEInteractionUpdate (cursor, ray, view) instead of ScreenDelta alone."))
	void UpdateInteraction(const FDIVEInteractionUpdate& Update);
	virtual void UpdateInteraction_Implementation(const FDIVEInteractionUpdate& Update);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Action")
	void EndInteraction(bool bCommit);
	virtual void EndInteraction_Implementation(bool bCommit);

	UFUNCTION(BlueprintCallable, Category = "Action")
	bool IsInteractionActive() const { return bInteractionActive; }

	/** Called by the session before Begin so NotifyInteractionValue / NotifyValueChanged in Begin has Context. */
	void MarkInteractionActive(const FDIVEActionContext& Context = FDIVEActionContext());

	UFUNCTION(BlueprintCallable, Category = "Action", meta = (
		ToolTip = "Broadcast OnValueChanged with Normalized only (Unit = None). Context is the one from Begin."))
	void NotifyValueChanged(float NormalizedValue);

	UFUNCTION(BlueprintCallable, Category = "Action", meta = (
		ToolTip = "Broadcast OnValueChanged with Absolute + Unit for the value HUD. Normalized is clamped 0..1."))
	void NotifyInteractionValue(const FDIVEInteractionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Action", meta = (
		ToolTip = "Self-complete; session closes the continuous slot on the next update."))
	void NotifyInteractionCompleted();

	virtual bool Execute_Implementation(const FDIVEActionContext& Context) override;

protected:
	UPROPERTY(Transient)
	bool bInteractionActive = false;

	UPROPERTY(Transient)
	FDIVEActionContext ActiveInteractionContext;
};

USTRUCT(BlueprintType)
struct DIVECORE_API FDIVEContextMenuEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	TObjectPtr<UDIVEDeviceAction> Action = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	bool bEnabled = true;

	/** True when the action is in a "checked" (active/on) state. Widgets should render a checkmark or similar indicator. */
	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	bool bChecked = false;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	FName TargetKey = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	FName BindingId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "DIVE")
	bool bIsSeparator = false;
};
