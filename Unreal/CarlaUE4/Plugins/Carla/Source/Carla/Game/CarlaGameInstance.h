// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "Engine/GameInstance.h"
#include "NetworkReplayStreaming.h"

#include "Game/CarlaGameControllerBase.h"
#include "Game/DataRouter.h"
#include "Vehicle/VehicleSpawnerBase.h"
#include "Vehicle/CarlaWheeledVehicle.h"
#include "Settings/CarlaSettings.h"
#include "Walker/WalkerSpawnerBase.h"

#include "CarlaGameInstance.generated.h"

class UCarlaSettings;
struct FMockGameControllerSettings;

/// Traffic density
UENUM(BlueprintType)
enum class ETrafficDensity : uint8
{
	Low		UMETA(DisplayName = "Low"),
	Normal 	UMETA(DisplayName = "Normal"),
	High	UMETA(DisplayName = "High"),
};



// stuff from the unreal replay tutorial
USTRUCT(BlueprintType)
struct FS_ReplayInfo
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString ReplayName;

    UPROPERTY(BlueprintReadOnly)
    FString FriendlyName;

    UPROPERTY(BlueprintReadOnly)
    FDateTime Timestamp;

    UPROPERTY(BlueprintReadOnly)
    int32 LengthInMS;

    UPROPERTY(BlueprintReadOnly)
    bool bIsValid;

    FS_ReplayInfo(FString NewName, FString NewFriendlyName, FDateTime NewTimestamp, int32 NewLengthInMS)
    {
      ReplayName = NewName;
      FriendlyName = NewFriendlyName;
      Timestamp = NewTimestamp;
      LengthInMS = NewLengthInMS;
      bIsValid = true;
    }

    FS_ReplayInfo()
    {
      ReplayName = "Replay";
      FriendlyName = "Replay";
      Timestamp = FDateTime::MinValue();
      LengthInMS = 0;
      bIsValid = false;
    }
};

/// The game instance contains elements that must be kept alive in between
/// levels. It is instantiate once per game.
UCLASS()
class CARLA_API UCarlaGameInstance : public UGameInstance
{
  GENERATED_BODY()

public:

  UCarlaGameInstance();

  ~UCarlaGameInstance();

  void InitializeGameControllerIfNotPresent(const FMockGameControllerSettings &MockControllerSettings);

  ICarlaGameControllerBase &GetGameController()
  {
    check(GameController != nullptr);
    return *GameController;
  }

  UCarlaSettings &GetCarlaSettings()
  {
    check(CarlaSettings != nullptr);
    return *CarlaSettings;
  }

  const UCarlaSettings &GetCarlaSettings() const
  {
    check(CarlaSettings != nullptr);
    return *CarlaSettings;
  }

  // Extra overload just for blueprints.
  UFUNCTION(BlueprintCallable)
  UCarlaSettings *GetCARLASettings()
  {
    return CarlaSettings;
  }

  FDataRouter &GetDataRouter()
  {
    return DataRouter;
  }

  UFUNCTION(BlueprintPure, Category = "Replays")
  bool IsRecordingReplay()
  {
  	return isRecording;
  }

  bool IsPlayingReplay()
  {
  	return isReplay;
  }

  	// gets pawn tagged with player tag
  APawn* GetPlayerPawn();

  APawn* GetPlayerPawnByTag();

  APawn* GetPlayerPawnByName();

  virtual void PlayReplay(const FString&, UWorld*, const TArray<FString>&) override;

  virtual void StartRecordingReplay(const FString& InName, const FString& FriendlyName, const TArray<FString>& AdditionalOptions = TArray<FString>()) override;

  virtual void StopRecordingReplay() override;

  // see https://wiki.unrealengine.com/Replay_System_Tutorial
  // for where these functions came from
  /** Start recording a replay from blueprint. ReplayName = Name of file on disk, FriendlyName = Name of replay in UI */
  UFUNCTION(BlueprintCallable, Category = "Replays")
  void StartRecordingReplayFromBP(FString ReplayName, FString FriendlyName);

  /** Start recording a running replay and save it, from blueprint. */
  UFUNCTION(BlueprintCallable, Category = "Replays")
  void StopRecordingReplayFromBP();

  /** Start playback for a previously recorded Replay, from blueprint */
  UFUNCTION(BlueprintCallable, Category = "Replays")
  void PlayReplayFromBP(FString ReplayName);

  /** Start looking for/finding replays on the hard drive */
  UFUNCTION(BlueprintCallable, Category = "Replays")
  void FindReplays();

  /** Apply a new custom name to the replay (for UI only) */
  UFUNCTION(BlueprintCallable, Category = "Replays")
  void RenameReplay(const FString &ReplayName, const FString &NewFriendlyReplayName);

  /** Delete a previously recorded replay */
  UFUNCTION(BlueprintCallable, Category = "Replays")
  void DeleteReplay(const FString &ReplayName);

  virtual void Init() override;

  // plays a demofile and saves the data from the sensors
  UFUNCTION(BlueprintCallable, Category = "Replays")
  void DemoRenderFrames(const FString& demoName, int fps);

//  // stuff for multiple replays
//	void HandlePreLoadMap(const FString& MapName);
//
//	FDelegateHandle HandlePreLoadMapDelegate;


  bool CaptureFrames() const
  {
  	return spectatorToCaptureFrames;
  }

  FString* GetCaptureFolder() const
  { return captureFolder;}

  FString* GetDemoName() const
  { return demoFileName; }

  void SetCaptureFolder(const FString& folder);

  int GetCaptureFPS() const
  {return captureFPS;}

  int GetDemoState() const
  {
	  return demoStatus;
  }


  FDateTime demoStartTime;

  // for fixed navigation sequences
  void IncrementIndexInNavigationSequence()
  {
  	indexInNavigationSequence++;
  }

  void SetIndexInNavigationSequence(int newIndex)
  {
    indexInNavigationSequence = newIndex;
  }

  int IndexInNavigationSequence() const
  {
  	return indexInNavigationSequence;
  }

  // uses the index in the navigation sequence to index into the ordering array
  int GetCurrentNavigationDestination() const ;

  void SetStartIndex(int index);

  virtual void Shutdown() override;

  // Auto rendering
//	void RenderNext();

	UFUNCTION()
	virtual void BeginLoadingScreen(const FString& MapName);

	UFUNCTION()
	virtual void EndLoadingScreen(UWorld* InLoadedWorld);

	EExperimentType GetExperimentType() const
	{
		return experimentType;
	}

private:

	UPROPERTY(Category = "CARLA Settings", EditAnywhere)
	UCarlaSettings *CarlaSettings;

	FDataRouter DataRouter;

	TUniquePtr<ICarlaGameControllerBase> GameController;

	friend class ACarlaSpectatorController;

	EExperimentType experimentType = EExperimentType::Navigation;

	bool isReplay = false;
	bool isRecording = false;
	int demoStatus = 0;	// 0 for no demo, 1 for demo recording, 2 for demo ended
	bool spectatorToCaptureFrames = false;
	FString* captureFolder = nullptr;
	FString* demoFileName = nullptr;
	int captureFPS = 0;

	// more things from the unreal engine replay tutorial
	// for FindReplays()
	TSharedPtr<INetworkReplayStreamer> EnumerateStreamsPtr;
	FOnEnumerateStreamsComplete OnEnumerateStreamsCompleteDelegate;

	void OnEnumerateStreamsComplete(const TArray<FNetworkReplayStreamInfo>& StreamInfos);

	// for DeleteReplays(..)
	FOnDeleteFinishedStreamComplete OnDeleteFinishedStreamCompleteDelegate;

	void OnDeleteFinishedStreamComplete(const bool bDeleteSucceeded);

	APawn* playerPawn;

	FName* playerPawnName;	// because apparently pawns get switched up and a pointer to a found pawn at the beginning of a replay is not persistent

//	ACarlaSpectatorController* spectatorController = nullptr;	// because I'm having trouble controlling replays from the player controller

	int RequestOBSRecording(bool startStop);	// tries to send message to obs - true to start recording, false to stop

	int GetIndexOfClosestPlayerStart(const FVector &location);

	// ============================
	// for spawning cars on the fly
	// cases where traffic gets backed up so we clear those actors and respawn them
	// ============================
private:
	AVehicleSpawnerBase *vehicleSpawner = nullptr;

	AWalkerSpawnerBase *pedestrianSpawner = nullptr;

	void FindVehicleSpawner();

	void FindPedestrianSpawner();

	int nVehiclesToSpawn = 0;

	float secondsToNextRespawnAttempt = 1.0;

public:
	void AddOneVehicleToRespawnQueue(ACarlaWheeledVehicle *vehicleToReincarnate);

	void ClearVehicleSpawnerPointer();  // needed on a restart and I couldn't figure out what function could be overridden

	void ClearPedestrianSpawnerPointer();  // needed on a restart and I couldn't figure out what function could be overridden

public:
	ETrafficDensity GetTrafficDensity() const {return currentTrafficDensity;};

	// returns number of cars at this density
	int SetTrafficDensity(ETrafficDensity newDensity);

	// Called when settings are updated from the menu
	void ReloadSettings();

private:
	// for experiments that change the traffic, i.e. track running
	// use these default values to reset the corresponding values to defaults
	// the distance us -1 because 0 max distance means no limit
	// distance in meters
	void SetTraffic(int numberOfCars = -1, float minSpawnDistance = -1, float maxSpawnDistance = -1, float maxSpawnAngle = 0, int numberToSpawn = 0);

	ETrafficDensity currentTrafficDensity = ETrafficDensity::Normal;

  //========================================
  // for reading fixed navigation sequences
  //========================================
public:
	void LoadNavigationSequence(const FString& navigationSequenceFile);

private:
	TArray<int> navigationSequence;
	int indexInNavigationSequence = 0;

	// for keeping state between runs for subjects
public:
	void LoadSubjectStateFile();
	void SaveSubjectStateFile();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Replays")
	void BP_OnFindReplaysComplete(const TArray<FS_ReplayInfo> &AllReplays);

	// for rendering all demos
	TArray<FString> allDemos;

	// has each demo been rendered? is a check for the positions xml file
	TArray<bool> isDemoRendered;

public:
// For menu of replay rendering stuff because 1) Shipping builds don't have console, 2) Text-based interfaces are a relic of the 80s
	UFUNCTION(BlueprintPure, Category = "Replays")
	const TArray<FString> &GetDemosList();

	UFUNCTION(BlueprintPure, Category = "Replays")
	const TArray<bool> &GetIsDemoRenderedList()
	{
		return isDemoRendered;
	}

// because the resolution must be set by a player controller, there might be multiple calls
// and we prevent that by having a one-time flag in the game instance
private:
	bool bIsResolutionSet = false;

	// load demo list only when needed elase the call stack does this a lot of times
	bool bIsDemosListStale = true;

public:
	UPROPERTY(BlueprintReadOnly, Category = "fMRI")
	FString replayToRender = FString();

public:
	bool HasResolutionBeenSet() const
	{
		return bIsResolutionSet;
	}

	void UpdateResolutionHasBeenSet()
	{
		bIsResolutionSet = true;
	}

public:
	bool IsAutoRender() const
	{
		return bIsAutoRender;
	}
	
private:
	bool bIsAutoRender = false;
};
