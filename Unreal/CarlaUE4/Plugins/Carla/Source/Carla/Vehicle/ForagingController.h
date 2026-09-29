// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "Carla.h"
#include "CoreMinimal.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Game/ForagingPlayerState.h"
#include "Util/AdaptiveTrialParameterGenerator.h"
#include "ForagingController.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API AForagingController : public ANavigationVehicleController
{
	GENERATED_BODY()

public:
	AForagingController(const FObjectInitializer& ObjectInitializer);

	/**
	 * Get the number of targets the subject has to collect to complete the trial
	 * May be fewer than the number of active targets.
	 * @return 
	 */
	UFUNCTION(BlueprintPure)
	virtual int GetNumberOfTargets() const;

	UFUNCTION(BlueprintPure)
	virtual int GetNumberOfVisitedTargets() const
	{
		return visitedCount;
	} // SJ: why is this defined within header

	virtual void BeginPlay() override;

	virtual void Possess(APawn *aPawn) override;

	virtual void ExperimentTick(float) override;

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void PickNextDestination(int minIndex = 0, int maxIndex = -1) override;

	virtual void ConfigureNextDestination() override;

	/**
	 * Checks whether the subject arrived at _any_ of the foraging targets
	 */
	virtual bool CheckForArrival() override;

	virtual void OnSegmentEnd(float maxWait, bool isLost = false) override;

	virtual void TTLdown() override;

	// ticks every second by a timer to update a countdown clock
	void SecondTick();

	// allows subject to pause trials
	// useful for the ephys subject
	virtual void TogglePause();

	virtual void SetupInputComponent() override;

	virtual bool ShouldCheckOutOfBounds() const { return true; }

private:
	bool bIsVisitedCountStale = false;
	int visitedCount = 0;

	// randomized number of targets to use per trial
	int minNumTargets = 3;
	int maxNumTargets = 6;
	double targetAvailabilityMultiplier = 2.0;
	double valueResetProbability = 0.25;
	int minStableTrials = 3;	// minimum number of trials before resetting values
	int numStableTrials = 0;	// number of trials elapsed with these stable values

	int baseTrialTime = 90;
	int additionalItemTime = 15;
	int maxTrialTime = 180;

	int thisTrialDuration = 0;	// max duration of current trial
	/**
	 * Seconds elapsed since trial start to the most recent item collected
	 * Used to calculate on average how long it takes the subject to collect an item
	 */
	int timeElapsedToRecentPickup = 0;

	AdaptiveTrialParameterGenerator parameterAdjust;

	AForagingPlayerState *foragingPlayerState;

	float collectionSpeed = 5;		// max speed at which subject will collect item in unreal speed

	// TODO: move these to MRIPlayerController
	bool isTimed = false;
	FTimerHandle secondTickTimer;
	int secondsRemaining;
};
