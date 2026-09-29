// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/TrackRunningPlayerState.h"
#include "NavigationExperimentState.h"

/**
 * 
 */
class CARLA_API TrackRunningExperimentState : public NavigationExperimentState
{
public:
	TrackRunningExperimentState(double time);

	TrackRunningExperimentState(double time, int frame);

	TrackRunningExperimentState(double time, int frame, TArray<const UAgentComponent*> agents);

	TrackRunningExperimentState(double time, int frame, TArray<const UAgentComponent*> agents, ATrackRunningPlayerState *playerState);

	bool IsRunningOnTrack() const
	{
		return bIsRunningOnTrack;
	}

	int GetNumberOfVehicles() const
	{
		return numberOfVehicles;
	}

	int GetTrackRunningDirection() const
	{
		return trackRunningDirection;
	}

	~TrackRunningExperimentState();

protected:
	virtual void WriteContents(std::ofstream &logFile, int &indentation) override;

private:
	bool bIsRunningOnTrack = true;
	int numberOfVehicles = 0;
	int trackRunningDirection = 0;
};
