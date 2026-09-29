// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "TrackRunningExperimentState.h"

TrackRunningExperimentState::TrackRunningExperimentState(double time)
	: NavigationExperimentState(time)
{
}

TrackRunningExperimentState::TrackRunningExperimentState(double time, int frame)
	: NavigationExperimentState(time, frame)
{

}


TrackRunningExperimentState::TrackRunningExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
	: NavigationExperimentState(time, frame, agents)
{

}

TrackRunningExperimentState::TrackRunningExperimentState(double time, int frame, TArray<const UAgentComponent*> agents, ATrackRunningPlayerState *playerState)
	: NavigationExperimentState(time, frame, agents, playerState)
{
	bIsRunningOnTrack = playerState->IsPlayerRunningOnTrack();
	numberOfVehicles = playerState->GetNumberOfVehicles();
	trackRunningDirection = playerState->GetTrackRunningDirection();
}


TrackRunningExperimentState::~TrackRunningExperimentState()
{
}


void TrackRunningExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
{
//	UE_LOG(LogFMRI, Log, TEXT("Track running experiment state write content"));
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Track-Running>" << endl;
	indentation++;
	WRITE_INDENTS(logFile, indentation);
	logFile << WRITE_SINGLE_TAG("IsRunningOnTrack", bIsRunningOnTrack) << endl;
	WRITE_INDENTS(logFile, indentation);
	logFile << WRITE_SINGLE_TAG("NumberOfVehicles", numberOfVehicles) << endl;
	WRITE_INDENTS(logFile, indentation);
	logFile << WRITE_SINGLE_TAG("Direction", trackRunningDirection) << endl;
	indentation--;
	WRITE_INDENTS(logFile, indentation);
	logFile << "</Track-Running>" << endl;
	NavigationExperimentState::WriteContents(logFile, indentation);
}