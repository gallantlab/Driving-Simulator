// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/NavigationPlayerState.h"
#include "ForagingPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API AForagingPlayerState : public ANavigationPlayerState
{
	GENERATED_BODY()

public:
	AForagingPlayerState();
	virtual void Reset() override;
	virtual void ResetExperimentState() override;
	virtual void CopyProperties(APlayerState *PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// need to override this because we have multiple possible destinations
	virtual int GetCurrentDestination() const override;

	TArray<int> GetCurrentForagingTargets() const
	{
		return currentForageTargets;
	}

	TArray<bool> GetIsTargetForaged() const
	{
		return isTargetForaged;
	}

	int GetNumTargetsToCollect() const
	{
		return NumTargetsToCollect;
	}

	TArray<int> GetTargetValues() const
	{
		return targetValues;
	}

	UFUNCTION(BlueprintPure)
	bool IsPaused() const
	{
		return paused;
	}

	bool GetTrialDuration() const
	{
		return trialDuration;
	}

	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<int> targetValues;				// what are the point values of these targets on this trial

	// allows subjects to pause things between trials
	UPROPERTY(VisibleAnywhere, Replicated)
	bool paused = false;
protected:
	/**
	 * Number of targets to collect to end the trial. May be less than the number of active targets
	 * so that the subject doesn't have to collect all active targets to finish. this prevents them
	 * from having to wander around too much
	 */
	UPROPERTY(VisibleAnywhere, Replicated)
	int NumTargetsToCollect = 0;			// number of targets to collect, may be less than number of active targets

	UPROPERTY(VisibleAnywhere, Replicated)
	int trialDuration = 0;					// max duration of this trial

private:
	friend class AForagingController;

	// TODO: consider instead of using the bool array marker,
	// pop each target out of the array upon foraged
	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<int> currentForageTargets;		// indices of the destinations that are current targets

	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<bool> isTargetForaged;			// markers for whether each current target has been foraged
};
