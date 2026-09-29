// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "CircuitExperimentState.h"

CircuitExperimentState::CircuitExperimentState(double time)
	: NavigationExperimentState(time)
{
}

CircuitExperimentState::~CircuitExperimentState()
{
}

CircuitExperimentState::CircuitExperimentState(double time, int frame)
	: NavigationExperimentState(time, frame)
{

}


CircuitExperimentState::CircuitExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
	: NavigationExperimentState(time, frame, agents)
{

}


CircuitExperimentState::CircuitExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
													 ACircuitPlayerState *playerState)
	: NavigationExperimentState(time, frame, agents, playerState)
{
	currentCircuit = playerState->GetCurrentCircuit();
	nLapsDesired = playerState->GetDesiredLaps();
	nLapsCompleted = playerState->GetCompletedLaps();
	isOnCourse = playerState->GetIsOnCourse();
}


void CircuitExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
{
//	UE_LOG(LogFMRI, Log, TEXT("Circuit experiment state write content"));
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Circuit-Running>" << endl;
	indentation++;
	WRITE_INDENTS(logFile, indentation)
	logFile << WRITE_SINGLE_TAG("CurrentCircuit", currentCircuit) << endl;
	WRITE_INDENTS(logFile, indentation)
	logFile << WRITE_SINGLE_TAG("LapsDesired", nLapsDesired) << endl;
	WRITE_INDENTS(logFile, indentation)
	logFile << WRITE_SINGLE_TAG("LapsCompleted", nLapsCompleted) << endl;
	WRITE_INDENTS(logFile, indentation)
	logFile << WRITE_SINGLE_TAG("IsOnCourse", isOnCourse) << endl;
	indentation--;
	WRITE_INDENTS(logFile, indentation)
	logFile << "</Circuit-Running>" << endl;
	NavigationExperimentState::WriteContents(logFile, indentation);
}