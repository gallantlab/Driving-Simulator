// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/ForagingPlayerState.h"
#include "LearningPracticePlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ALearningPracticePlayerState : public AForagingPlayerState
{
	GENERATED_BODY()
public:
	ALearningPracticePlayerState();
	virtual void Reset() override;
	virtual void ResetExperimentState() override;
	virtual void CopyProperties(APlayerState *PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// need to override this because we have multiple possible destinations
	virtual int GetCurrentDestination() const override;

	TArray<int> GetCurrentTargets() const
	{
		return currentTargets;
	}

	TArray<bool> GetIsTargetVisited() const
	{
		return isTargetVisited;
	}

	TArray<int> GetTargetValues() const
	{
		return targetValues;
	}
	// TODO: consider instead of using the bool array marker,
	// pop each target out of the array upon foraged
	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<int> currentTargets;		// indices of the destinations that are current targets

	UPROPERTY(VisibleAnywhere, Replicated)
	TArray<bool> isTargetVisited;			// markers for whether each current target has been foraged

private:
	friend class ALearningPracticeController;

};
