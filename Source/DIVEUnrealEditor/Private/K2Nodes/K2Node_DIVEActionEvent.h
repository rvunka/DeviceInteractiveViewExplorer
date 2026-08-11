// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "EdGraph/EdGraphNodeUtils.h"
#include "K2Node.h"
#include "K2Node_EventNodeInterface.h"
#include "BlueprintNodeSpawner.h"

#include "K2Node_DIVEActionEvent.generated.h"

class UDIVEDeviceAction;
class FBlueprintActionDatabaseRegistrar;
class FKismetCompilerContext;
struct FBlueprintNodeSignature;

/**
 * Spawner that jumps to an existing DIVE Action Event for the same ActionClass
 * instead of placing a duplicate (Enhanced Input pattern).
 */
UCLASS(Transient)
class UDIVEActionEventNodeSpawner : public UBlueprintNodeSpawner
{
	GENERATED_BODY()

public:
	static UDIVEActionEventNodeSpawner* Create(
		TSubclassOf<UEdGraphNode> NodeClass,
		TSubclassOf<UDIVEDeviceAction> InActionClass);

	virtual UEdGraphNode* Invoke(
		UEdGraph* ParentGraph,
		FBindingSet const& Bindings,
		FVector2D const Location) const override;

private:
	UK2Node* FindExistingNode(const UBlueprint* Blueprint) const;

	UPROPERTY()
	TSubclassOf<UDIVEDeviceAction> ActionClass;
};

/** EI-like event: pick a DIVE Device Action class; fires when that action succeeds on this device. */
UCLASS()
class UK2Node_DIVEActionEvent : public UK2Node, public IK2Node_EventNodeInterface
{
	GENERATED_BODY()

public:
	UK2Node_DIVEActionEvent(const FObjectInitializer& ObjectInitializer);

	UPROPERTY(EditAnywhere, Category = "DIVE")
	TSubclassOf<UDIVEDeviceAction> ActionClass;

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

private:
	FName GetActionName() const;

	FNodeTextCache CachedTooltip;
	FNodeTextCache CachedNodeTitle;
};
