// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphNodeUtils.h"
#include "K2Node.h"
#include "K2Node_EventNodeInterface.h"
#include "BlueprintNodeSpawner.h"
#include "UObject/SoftObjectPath.h"

#include "K2Node_DIVEActionValueEvent.generated.h"

class UDIVEContinuousDeviceAction;
class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;
struct FBlueprintNodeSignature;

/**
 * Spawner that jumps to an existing wildcard DIVE Action Value Event (same ActionClass, empty BindingId)
 * instead of placing a duplicate.
 */
UCLASS(Transient)
class UDIVEActionValueEventNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_BODY()

public:
	static UDIVEActionValueEventNodeSpawner* Create(
		TSubclassOf<UEdGraphNode> NodeClass,
		const FSoftClassPath& InActionClassPath);

	virtual UEdGraphNode* Invoke(
		UEdGraph* ParentGraph,
		FBindingSet const& Bindings,
		FVector2D const Location) const override;

	virtual FBlueprintNodeSignature GetSpawnerSignature() const override;

private:
	UK2Node* FindExistingNode(const UBlueprint* Blueprint) const;
	UClass* ResolveActionClass(bool bLoadIfNeeded) const;

	UPROPERTY()
	TSubclassOf<UDIVEContinuousDeviceAction> ActionClass;

	UPROPERTY()
	FSoftClassPath ActionClassPath;
};

/**
 * Event node: pick a continuous DIVE Device Action class; fires on live value changes
 * while that action drives a gesture on this actor's Inspectable.
 */
UCLASS()
class UK2Node_DIVEActionValueEvent : public UK2Node, public IK2Node_EventNodeInterface
{
	GENERATED_BODY()

public:
	UK2Node_DIVEActionValueEvent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, Category = "DIVE")
	TSubclassOf<UDIVEContinuousDeviceAction> ActionClass;

	UPROPERTY(EditAnywhere, Category = "DIVE", meta = (
		GetOptions = "GetAvailableBindingIds",
		ToolTip = "None = any instance of this action class. Non-empty = only the matching BindingId."))
	FName BindingId;

	//~ UEdGraphNode
	virtual void AllocateDefaultPins() override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FText GetTooltipText() const override;
	virtual FSlateIcon GetIconAndTint(FLinearColor& OutColor) const override;
	virtual bool IsCompatibleWithGraph(UEdGraph const* Graph) const override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UEdGraphNode

	//~ UK2Node
	virtual void ValidateNodeDuringCompilation(FCompilerResultsLog& MessageLog) const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetMenuCategory() const override;
	virtual FBlueprintNodeSignature GetSignature() const override;
	virtual bool HasExternalDependencies(TArray<UStruct*>* OptionalOutput) const override;
	//~ End UK2Node

	//~ IK2Node_EventNodeInterface
	virtual TSharedPtr<FEdGraphSchemaAction> GetEventNodeAction(const FText& ActionCategory) override;
	//~ End IK2Node_EventNodeInterface

	UFUNCTION()
	TArray<FName> GetAvailableBindingIds() const;

private:
	FName GetActionName() const;

	FNodeTextCache CachedNodeTitle;
};
