// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "Carla.h"
#include "CoreMinimal.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Settings/CarlaSettings.h"
#include "TrackedLearningController.generated.h"

const FString SESSIONEND = FString("End of session. Please wait for instructions.");

/**
 * A class that keeps track of the number of destinations the subject has been to
 * and will always randomly generate destinations without distance constraints
 */
UCLASS()
class CARLA_API ATrackedLearningController : public ANavigationVehicleController
{
	GENERATED_BODY()

public:
	ATrackedLearningController(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintPure)
	int GetNumberOfActiveDestinations()
	{
		if (bIsActiveCountStale)
		{
			numActiveDestinations = destinations->GetNumberOfActiveDestinations();
			bIsActiveCountStale = false;
		}
		return numActiveDestinations;
	}

	UFUNCTION(BlueprintPure)
	int GetNumberOfVisitedActiveDestinations()
	{
		if (bIsNumVisitedStale)
		{
			numDestinationsVisited = destinations->GetNumberOfVisitedDestinations();
			bIsNumVisitedStale = false;
		}
		return numDestinationsVisited;
	}

	UFUNCTION(BlueprintCallable)
	void ResetVisitedDestinations();

	/**
	 * Overriden from NavigationVehicleController so that we can record that we've been here before
	 * and tchange the destination picking mode if needed
	 * @param maxWait
	 */
	virtual void OnSegmentEnd(float maxWait, bool isLost = false) override;

	virtual void ReloadActiveDestinations() override;

	virtual void BeginPlay() override;

	virtual void ExperimentTick(float) override;

protected:
	virtual void PickNextDestination(int minIndex = 0, int maxIndex = -1) override;
	
private:
	// if set to true, when randomly picking the next destination, will not
	// pick one that has already been visited until all active destinations
	// have been visited
	EDestinationPickingMode destinationPickingMode = EDestinationPickingMode::PurePermuted;

	bool bIsActiveCountStale = true;
	int numActiveDestinations = 0;

	bool bIsNumVisitedStale = true;
	int numDestinationsVisited = 0;

	FString visitationSaveFileName;

	float segmentEndDelay = -1.0f;
};
