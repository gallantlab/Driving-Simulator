// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "Carla.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Vehicle/NavigationVehicleController.h"
#include "CarlaUMGBase.generated.h"


/**
 * Base class for UMG-based experiment HUDs
 */
UCLASS()
class CARLA_API UCarlaUMGBase : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UCarlaUMGBase(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	int32 GetCurrentSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	int32 GetOnesSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	int32 GetTensSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	bool IsTTL() const;

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	float GetFPS() const;

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	int GetSpeedLimitMPH() const;

	UFUNCTION(BlueprintCallable, Category = "Driving HUD")
	virtual void FindController();

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	FVector GetPlayerViewPosition();

	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	APawn* GetPlayerPawn();

	UPROPERTY(BlueprintReadWrite, Category = "Driving HUD")
	bool showLocation =	// needs to be updated by according to the settings
#ifdef SHOW_POSITION
		true
#else
		false
#endif
		;


	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	ACarlaVehicleController* GetController();

	static const int eyetrackingCalibrationOrder[];

	UPROPERTY(BlueprintReadOnly)
	TArray<int> EyetrackingCalibrationOrder;

	UFUNCTION(BlueprintCallable, Category = "Driving HUD")
	void LogToTerminal(const FString& logMessage);

	// get a scale factor to offset the DPI scaling done by UMG
	// so we can put the eyetracking calibration dots in the right place
	UFUNCTION(BlueprintPure, Category = "Driving HUD")
	float DPIScaleFactor();

	/**
	 * Tell the player controller that settings have changed and it should reload things
	 */
	UFUNCTION(BlueprintCallable, Category = "Driving HUD")
	void PlayerControllerReloadSettings();

protected:
	ACarlaVehicleController* controller;

	double CalculatedDPIScaleFactor = -1;
};
