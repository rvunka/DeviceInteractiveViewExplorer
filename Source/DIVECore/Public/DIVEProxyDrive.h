// Copyright (c) 2026. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DIVEProxyDriveTypes.h"
#include "UObject/Interface.h"

#include "DIVEProxyDrive.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UDIVEProxyDrive : public UInterface
{
	GENERATED_BODY()
};

class DIVECORE_API IDIVEProxyDrive
{
	GENERATED_IINTERFACE_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool CanProxyDrive() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	bool BeginProxyDrive(const FDIVEProxyDriveContext& Context);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	void ApplyProxyDriveDelta(FVector2D ScreenDelta);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "DIVE|ProxyDrive")
	void EndProxyDrive(bool bCommit);
};
