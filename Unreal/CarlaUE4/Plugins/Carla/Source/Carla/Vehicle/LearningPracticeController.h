// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/ForagingController.h"
#include "Game/LearningPracticePlayerState.h"
#include "Math/RandomStream.h"
#include "LearningPracticeController.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ALearningPracticeController : public AForagingController
{
	GENERATED_BODY()

public:
	ALearningPracticeController(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	int GetRandomActiveDestination();

	void FindCurrentDestination();

	virtual void Possess(APawn *aPawn) override;

	virtual void ExperimentTick(float) override;

	virtual int GetNumberOfTargets() const override;

	int GetNumberOfVisitedTargets() const override;

	UFUNCTION(BlueprintPure)
	int GetNumberOfActiveDestinations()
	{
		if (bIsNumActiveStale)
		{
			bIsNumActiveStale = false;
			if (numTargets > destinations->GetNumberOfActiveDestinations()) {
				numActiveDestinations = destinations->GetNumberOfActiveDestinations();
			}
			else {
				numActiveDestinations = numTargets;
			}
		}
		return numActiveDestinations;
	}

	UFUNCTION(BlueprintPure)
	int GetNumberOfVisitedActiveDestinations()
	{
		if (bIsNumVisitedStale)
		{
			numVisited = GetNumberOfVisitedTargets();
			bIsNumVisitedStale = false;
		}
		return numVisited;
	}

	UFUNCTION(BlueprintCallable)
	void ResetVisitedDestinations();

	void ReloadActiveDestinations() override;
protected:
	/**
	 * Picks the next destination from the destination parser component
	 * and sets it in the player state
	 */
	virtual void ConfigureNextDestination() override;

	/**
	 * Checks whether the subject arrived at _any_ of the foraging targets
	 */
	virtual int CheckArrival();

	virtual void OnSegmentEnd(float maxWait, bool isLost = false) override;

	virtual void TTLdown() override;

	// allows subject to pause trials
	// useful for the ephys subject
	virtual void TogglePause();

	virtual void SetupInputComponent() override;

	bool destinationPoints = false;

	int numTargets = 80;

	float collectionSpeed = 5;		// max speed at which subject will collect item in unreal speed

	bool bIsNumVisitedStale = true;

	bool bIsNumActiveStale = true;

	int numVisited = 0;

	int numActiveDestinations = 0;

	FString visitationSaveFileName;

	bool loadedVisited = false;

	float segmentEndDelay = -1.0f;

private:
	FRandomStream randomStream;

	ALearningPracticePlayerState *learningPracticePlayerState;
};
