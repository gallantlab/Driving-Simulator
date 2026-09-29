// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "NavigationExperimentState.h"

NavigationExperimentState::NavigationExperimentState(double time)
	: ExperimentState(time)
{
}

NavigationExperimentState::~NavigationExperimentState()
{
}

NavigationExperimentState::NavigationExperimentState(double time, int frame)
	: ExperimentState(time, frame)
{

}


NavigationExperimentState::NavigationExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
	: ExperimentState(time, frame, agents)
{

}


NavigationExperimentState::NavigationExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
						  ANavigationPlayerState *playerState)
	: ExperimentState(time, frame, agents, playerState)
{
	this->destination = playerState->GetCurrentDestination();
	this->isLost = playerState->IsLost();
	this->isShowingHelp = playerState->IsShowingHelp();
}


void NavigationExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
{
//	UE_LOG(LogFMRI, Log, TEXT("Navigation xperiment state write content"));
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Destination" << WRITE_ATTRIBUTE_KV("id", destination) << "/>" << endl;
	if (isShowingHelp)
	{
		WRITE_INDENTS(logFile, indentation);
		logFile << "<Showing-Help/>" << endl;
	}
	if (isLost)
	{
		WRITE_INDENTS(logFile, indentation);
		logFile << "<Lost/>" << endl;
	}
	ExperimentState::WriteContents(logFile, indentation);
}