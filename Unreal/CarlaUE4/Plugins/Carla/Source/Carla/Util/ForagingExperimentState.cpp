// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "ForagingExperimentState.h"

#include <iostream>
#include <fstream>

ForagingExperimentState::ForagingExperimentState(double time)
	: NavigationExperimentState(time)
{
}

ForagingExperimentState::~ForagingExperimentState()
{
}

ForagingExperimentState::ForagingExperimentState(double time, int frame)
	: NavigationExperimentState(time, frame)
{

}


ForagingExperimentState::ForagingExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
	: NavigationExperimentState(time, frame, agents)
{

}


ForagingExperimentState::ForagingExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
	AForagingPlayerState *playerState)
	: NavigationExperimentState(time, frame, agents, playerState)
{
	this->foragingTargets = playerState->GetCurrentForagingTargets();
	this->isTargetForaged = playerState->GetIsTargetForaged();
	this->NumTargetsToCollect = playerState->GetNumTargetsToCollect();
	this->targetValues = playerState->GetTargetValues();
	this->paused = playerState->IsPaused();
	this->trialDuration = playerState->GetTrialDuration();
}

void ForagingExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
{
	if (paused)
	{
		WRITE_INDENTS(logFile, indentation);
		logFile << "<Paused/>" << endl;
	}
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Num-To-Collect>" << NumTargetsToCollect << "</Num-To-Collect>" << endl;
	logFile << "<Trial-Duration>" << trialDuration << "</Trial-Duration>" << endl;
	for (int i = 0; i < this->foragingTargets.Num(); i++)
	{
		WRITE_INDENTS(logFile, indentation);
		logFile << "<Target" << WRITE_ATTRIBUTE_KV("id", this->foragingTargets[i]) << WRITE_ATTRIBUTE_KV("Found", this->isTargetForaged[i]);
		logFile << WRITE_ATTRIBUTE_KV("Value", this->targetValues[i]);
		logFile << "/>" << endl;
	}
	NavigationExperimentState::WriteContents(logFile, indentation);
}