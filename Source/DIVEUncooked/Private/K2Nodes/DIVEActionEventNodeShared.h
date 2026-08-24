// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Templates/Function.h"
#include "UObject/NameTypes.h"
#include "UObject/SoftObjectPath.h"

class FBlueprintActionDatabaseRegistrar;
class UBlueprint;
class UBlueprintNodeSpawner;
class UClass;

void DIVEUncooked_EnsureActionClassMenuRefreshHooks();
void DIVEUncooked_UninstallActionClassMenuRefreshHooks();

bool DIVEUncooked_IsUsableActionClass(const UClass* Class, const UClass* BaseClass);
bool DIVEUncooked_BlueprintHasInspectableComponent(const UBlueprint* Blueprint);
TArray<FName> DIVEUncooked_GetAvailableBindingIds(const UBlueprint* Blueprint);

void DIVEUncooked_RegisterActionClassMenuActions(
	const UClass* NodeClass,
	FBlueprintActionDatabaseRegistrar& ActionRegistrar,
	const UClass* ActionBaseClass,
	TFunctionRef<UBlueprintNodeSpawner*(const FSoftClassPath&)> CreateSpawner);
