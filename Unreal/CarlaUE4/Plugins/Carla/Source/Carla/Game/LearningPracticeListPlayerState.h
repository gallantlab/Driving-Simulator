// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/LearningPracticePlayerState.h"
#include "LearningPracticeListPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ALearningPracticeListPlayerState : public ALearningPracticePlayerState
{
	GENERATED_BODY()

public:
	ALearningPracticeListPlayerState();
	virtual void Reset() override;
	virtual void ResetExperimentState() override;
	virtual void CopyProperties(APlayerState *PlayerState) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	TArray<int> GetCurrentTargets() const
	{
		return currentTargets;
	}

	TArray<bool> GetIsTargetVisited() const
	{
		return isTargetVisited;
	}

	int GetNumTargetsToCollect() const
	{
		return NumTargetsToVisit;
	}

	TArray<int> GetTargetValues() const
	{
		return targetValues;
	}

	UFUNCTION(BlueprintPure)
	bool IsPaused() const {
		return paused;
	}

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "UI")
	bool bIsDestinationsListVisible = false;
protected:
	UPROPERTY(VisibleAnywhere, Replicated)
	int NumTargetsToVisit = 0;
private:
	friend class ALearningPracticeListController;

};
