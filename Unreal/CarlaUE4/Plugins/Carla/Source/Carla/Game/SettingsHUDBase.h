// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/CarlaDriveHUDBase.h"
#include "Util/DestinationParserComponent.h"
#include "SettingsHUDBase.generated.h"


/**
 * UMG settings menu base
 * used for (gah) both the main menu and submenu pages
 */
UCLASS()
class CARLA_API USettingsHUDBase : public UCarlaDriveHUDBase
{
	GENERATED_BODY()

public:
	USettingsHUDBase(const FObjectInitializer& initializer);
	
	UFUNCTION(BlueprintCallable, Category = "Settings HUD")
	void UpdateActiveDestinationsInController();

	UFUNCTION(BlueprintPure, Category = "Settings HUD")
	UDestinationParserComponent *GetDestinationParserComponent();

	UFUNCTION(BlueprintPure, Category = "Settings HUD")
	int GetCurrentDestination();

	UFUNCTION(BlueprintCallable, Category = "Settings HUD")
	void ControllerRestartLevel();

	UFUNCTION(BlueprintPure, Category = "Settings HUD")
	const TArray<FString>& GetSavedConfigs();

	UFUNCTION(BlueprintCallable, Category = "Settings HUD")
	static FString GetFullSavePath(const FString& configFileName);

	UFUNCTION(BlueprintCallable, Category = "Settings HUD")
	void GenerateNextDestination();

	UPROPERTY(BlueprintReadWrite, Category = "Settings HUD")
	bool bIsSavedConfigsStale = true;

private:
	TArray<FString> SavedConfigs;

};
