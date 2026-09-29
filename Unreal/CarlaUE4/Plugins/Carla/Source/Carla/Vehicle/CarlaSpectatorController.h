// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/CarlaVehicleController.h"
#include "Sensor/SceneCaptureToDiskCamera.h"
#include "Game/TaggerDelegate.h"
#include "Util/ExperimentState.h"
#include "Util/DestinationParserComponent.h"
#include "Util/ExperimentLoggerComponent.h"

#include "CarlaSpectatorController.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ACarlaSpectatorController : public ACarlaVehicleController
{
	GENERATED_BODY()
	
public:
	ACarlaSpectatorController(const FObjectInitializer& objectInitializer);

	~ACarlaSpectatorController();

	// overridden Tick that doesn't call the TickAI thing
	virtual void Tick(float deltaTime) override;

	virtual void EnableUserInput(bool enable) override;

	virtual void SetAutopilot(bool Enable, bool ClearPlannedLocations) override;

	void SetReferencePawn(APawn* ref);

	// two post-spawning functions that might come in useful
	virtual void PostNetInit() override;

	virtual void PostActorCreated() override;

	// called after taking posession of a pawn
	virtual void BeginPlayingState() override;

	virtual void Possess(APawn *) override;

	virtual APawn* GetVehiclePawn() override;

	APawn* FindTaggedPlayerPawn();

	void CaptureSensors();			// save images from all sensors

	bool captureFrames;

	void SetFrameCaptureFolder(const FString& folder);

	void SaveExperimentLog();

	bool IsStale() {return stopDemo;}

	void AddExperimentState(ExperimentState *newState);

	ACarlaPlayerState * GetReferencePlayerState() const
	{
		return referencePlayerState;
	}

	UDemoNetDriver* GetDemoNetDriver() const
	{
		return demoNetDriver;
	}

	UCarlaGameInstance* GetCarlaGameInstance() const
	{
		return carlaGameInstance;
	}

	int GetFrameNumber() const
	{
		return frameNumber;
	}

protected:
	virtual void TickAutopilotController(float deltaTime) override;

	virtual void TagPawn() override;

	virtual void SetSteeringInput(float value) override;

	virtual void SetThrottleInput(float value) override;

	virtual void SetBrakeInput(float Value) override;

	virtual void HoldHandbrake() override;

	virtual void ReleaseHandbrake() override;


void TrySetViewTargetToPlayer();

	void FixTimeStep(bool);

	void LogExperimentState();

private:
	// can't possess a pawn, but need to follow one
	APawn* referencePawn = nullptr;

	UDestinationParserComponent *destinations = nullptr;

	UExperimentLoggerComponent *loggerComponent = nullptr;

	ACarlaPlayerState* referencePlayerState = nullptr;

	TArray<ASceneCaptureToDiskCamera*> *sensors = nullptr;

	bool needReference = true;

	int fps = 0;

	double currentTime = -1.0;

	double timeStep = 0.0;

	double delay = 5.0;	// delay between spawning and starting to render frames

	UDemoNetDriver *demoNetDriver = nullptr;

	AWorldSettings *worldSettings = nullptr;

	// experiment state corresponding to each rendered frame
	// the actual objects behind the pointers should be subclasses of ExperimentState
	TArray<ExperimentState*> *frameStates = nullptr;

	EExperimentType experimentType = EExperimentType::Other;

	int frameNumber;

	uint32 playerAgentID;

	UCarlaGameInstance* carlaGameInstance = nullptr;

	bool stopDemo = false;

	void SpawnLoggerComponent();

	UTaggerDelegate *taggerDelegate;	// need this to semantically tag vehicles that are spawned as part of traffic
};
