// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Game/TrackRunningPlayerState.h"
#include "Util/DestinationParserComponent.h"
#include "TrackRunningController.generated.h"

/**
 * Controller class for the track running experiment
 */
UCLASS()
class CARLA_API ATrackRunningController : public ANavigationVehicleController
{
	GENERATED_BODY()
	
public:
	ATrackRunningController(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	virtual void ExperimentTick(float dTime) override;

	virtual void SetFixedRoute(const TArray<FVector> &Locations, bool bOverwriteCurrent,
							   int newDirection) override;

	virtual void OnFixedRouteFinished() override;

	UFUNCTION(BlueprintCallable, Category = "Track running")
	void SetTrackRunningDirection(int direction);

	ETrafficDensity GetTrafficDensity() const ;

	virtual void Reset() override;

protected:
	// sets traffic conditions, etc, for next trial
	void ConfigureNextTrial();

	void SetIsRunningDownTrack(bool state);

	virtual void ResetExperimentState() override;

	virtual void PickNextDestination(int minIndex = 0, int maxIndex = -1) override;

	virtual void SetCurrentDestination(int newDestination, bool markerOff = true) override;

private:

	bool bIsRunningDownTrack = false;
	int currentDirection = 0;
	float secondsUntilNextInstruction = 0.0f;

	ATrackRunningPlayerState *trackRunningPlayerState;

	bool bIsTrialConfigured = false;

	int currentTraffic = -1;

	/**
	 * Persists until the beginning of the next trial, unlike the one in the NavigationPlayerState,
	 * which persists only until the end of the current trial and is -1 between trials.
	 * So this is only -1 at the beginning of a run
	 */
	int currentDestination = -1;

};
