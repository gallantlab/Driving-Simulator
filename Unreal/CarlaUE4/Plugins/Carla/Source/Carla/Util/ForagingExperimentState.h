// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "NavigationExperimentState.h"
#include "Game/ForagingPlayerState.h"

/**
 * Used for foraging experiments with multiple simultaneously visible targets
 */
class CARLA_API ForagingExperimentState : public NavigationExperimentState
{
public:
	ForagingExperimentState(double time);

	ForagingExperimentState(double time, int frame);

	ForagingExperimentState(double time, int frame, TArray<const UAgentComponent*> agents);

	ForagingExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
		AForagingPlayerState *playerState);

	~ForagingExperimentState();

	TArray<int> GetForagingTargets() const
	{
		return foragingTargets;
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

	bool IsPaused() const
	{
		return paused;
	}

	int GetTrialDuration() const
	{
		return trialDuration;
	}

protected:
	virtual void WriteContents(std::ofstream &logFile, int &indentation) override;

private:
	TArray<int> foragingTargets;
	TArray<bool> isTargetForaged;
	int NumTargetsToCollect;
	TArray<int> targetValues;
	bool paused;
	int trialDuration;
};
