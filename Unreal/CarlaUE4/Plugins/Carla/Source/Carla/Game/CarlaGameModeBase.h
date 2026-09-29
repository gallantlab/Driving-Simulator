// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/GameModeBase.h"

#include "DynamicWeather.h"
#include "Game/CarlaGameControllerBase.h"
#include "Game/CarlaGameInstance.h"
#include "Game/MockGameControllerSettings.h"
#include "Vehicle/VehicleSpawnerBase.h"
#include "Walker/WalkerSpawnerBase.h"
#include "Util/HumanStartZone.h"

#include "CarlaGameModeBase.generated.h"

class ACarlaVehicleController;
class APlayerStart;
class ASceneCaptureCamera;
class UCarlaGameInstance;
class UTaggerDelegate;
class UCarlaSettingsDelegate;
class ANavigationVehicleController;
class ANavigationPlayerState;
class ATrackRunningController;
class ATrackRunningPlayerState;
UCLASS(HideCategories=(ActorTick))
class CARLA_API ACarlaGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:

	ACarlaGameModeBase(const FObjectInitializer& ObjectInitializer);

	virtual void InitGame(const FString &MapName, const FString &Options, FString &ErrorMessage) override;

	virtual void RestartPlayer(AController *NewPlayer) override;

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	FDataRouter &GetDataRouter()
	{
		check(GameInstance != nullptr);
		return GameInstance->GetDataRouter();
	}

	UFUNCTION(BlueprintPure, Category="CARLA Settings")
	UCarlaSettingsDelegate *GetCARLASettingsDelegate()
	{
		return CarlaSettingsDelegate;
	}

	// plays a demofile while recording frames from all the sensors
	UFUNCTION(Exec, Category = ExecFunctions)
	void RenderDemoFrames(const FString& demoName, int fps);

	// takes screenshots using all the sensors on a spectator controller, if available
	UFUNCTION(Exec, Category = ExecFunctions)
	void CaptureSensors();

	// option, where to save captures?
	UFUNCTION(Exec, Category = ExecFunctions)
	void SetCaptureFolder(const FString& folder);

	// do eyetracking
	UFUNCTION(Exec, BlueprintImplementableEvent, Category = ExecFunctions)
	void CalibrateEyetracking();

	// interrupt eyetracking
	UFUNCTION(Exec, BlueprintImplementableEvent, Category = ExecFunctions)
	void TerminateEyetracking();
	
	// Tell the controller that eyetracking has ended
	UFUNCTION(BlueprintCallable, DisplayName = "Set controller eyetracking ended")
	void SetControllerEyetrackingEnded();

	// stuff for saving and loading subject info manually
	UFUNCTION(Exec, Category = ExecFunctions)
	void SaveSubjectState();

	UFUNCTION(Exec, Category = ExecFunctions)
	void LoadSubjectState();

	UFUNCTION(Exec, Category = ExecFunctions)
	void SetIndexInNavigationSequence(int newIndex);

	// debug exec function that marks the subject as arrived when called
	UFUNCTION(Exec, Category = ExecFunctions)
	void GenerateNextDestination();

	// Used by game instance to figure out which demo to render
	int GetIndexToRender() { return indexToRender++; };

	/// Use human start zones if present?
	/// If false, will not restrict player spawn points to those reserved for humans
	UPROPERTY(BlueprintReadWrite)
	bool bUseHumanStartZones = true;

	// Settings menu.
	UFUNCTION(BlueprintImplementableEvent)
	void SettingsMenu();

	// Open settings menu to the controls page
	UFUNCTION(BlueprintImplementableEvent)
	void ShowControls();

	UPROPERTY(BlueprintReadWrite)
	bool bIsSettingsMenuActive = false;

	UFUNCTION(BlueprintCallable)
	void VehicleSpawnerFindSpawnPoints();

protected:

	/** Used only when networking is disabled. */
	UPROPERTY(Category = "Mock CARLA Controller", EditAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Mock CARLA Controller"))
	FMockGameControllerSettings MockGameControllerSettings;

	/** The class of DynamicWeather to spawn. */
	UPROPERTY(Category = "CARLA Classes", EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ADynamicWeather> DynamicWeatherClass;

	/** The class of VehicleSpawner to spawn. */
	UPROPERTY(Category = "CARLA Classes", EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AVehicleSpawnerBase> VehicleSpawnerClass;

	/** The class of WalkerSpawner to spawn. */
	UPROPERTY(Category = "CARLA Classes", EditAnywhere, BlueprintReadOnly)
	TSubclassOf<AWalkerSpawnerBase> WalkerSpawnerClass;

	UPROPERTY(BlueprintReadOnly)
	EExperimentType experimentType = EExperimentType::Navigation;

private:

	void RegisterPlayer(AController &NewPlayer);

	void ParseDestinations();

	void AttachSensorsToPlayer();

	void TagActorsForSemanticSegmentation();

	/// Iterate all the APlayerStart present in the world and add the ones with
	/// unoccupied locations to @a UnOccupiedStartPoints.
	///
	/// @return APlayerStart if "Play from Here" was used while in PIE mode.
	APlayerStart *FindUnOccupiedStartPoints(
			AController *Player,
			TArray<APlayerStart *> &UnOccupiedStartPoints);

	ICarlaGameControllerBase *GameController;

	UPROPERTY()
	UCarlaGameInstance *GameInstance;

	UPROPERTY()
	ACarlaVehicleController *PlayerController;

	UPROPERTY()
	UTaggerDelegate *TaggerDelegate;

	UPROPERTY()
	UCarlaSettingsDelegate* CarlaSettingsDelegate;

	UPROPERTY()
	ADynamicWeather *DynamicWeather;

	UPROPERTY()
	AVehicleSpawnerBase *VehicleSpawner;

	UPROPERTY()
	AWalkerSpawnerBase *WalkerSpawner;

	// auto rendering all replays
	int indexToRender = 0;

	// Human-only start zones
	TArray<AHumanStartZone*> humanStartZones;

};
