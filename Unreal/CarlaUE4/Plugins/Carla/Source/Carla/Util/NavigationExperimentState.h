// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "ExperimentState.h"
#include "Game/NavigationPlayerState.h"

/**
 * Used in taxi driver navigation experiments
 */
class CARLA_API NavigationExperimentState : public ExperimentState
{
public:
	NavigationExperimentState(double time);

	NavigationExperimentState(double time, int frame);

	NavigationExperimentState(double time, int frame, TArray<const UAgentComponent*> agents);

	NavigationExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
							  ANavigationPlayerState *playerState);

	int GetDestination() const
	{
		return destination;
	}

	bool IsLost() const
	{
		return isLost;
	}

	bool IsShowingHelp()
	{
		return isShowingHelp;
	}

	~NavigationExperimentState();

protected:
	virtual void WriteContents(std::ofstream &logFile, int &indentation) override;

private:
	int destination = -1;

	bool isLost = false;

	bool isShowingHelp = false;
};
