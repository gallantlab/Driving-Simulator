// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "NavigationExperimentState.h"
#include "Game/CircuitPlayerState.h"

/**
 * Used for running in a close loop circuit
 */
class CARLA_API CircuitExperimentState : public NavigationExperimentState
{
public:
	CircuitExperimentState(double time);
	
	CircuitExperimentState(double time, int frame);
	
	CircuitExperimentState(double time, int frame, TArray<const UAgentComponent*> agents);

	CircuitExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
						   ACircuitPlayerState *playerState);

	~CircuitExperimentState();


protected:
	virtual void WriteContents(std::ofstream &logFile, int &indentation) override;

private:
	int currentCircuit = -1;
	int nLapsDesired = -1;
	int nLapsCompleted = -1;
	int isOnCourse = -2;
};
