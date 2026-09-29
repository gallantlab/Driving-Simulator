// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.
//
// Created by sjshim on 7/17/25.
//

#include "Carla.h"
#include "LearningPracticeListExperimentState.h"

#include <iostream>
#include <fstream>

LearningPracticeListExperimentState::LearningPracticeListExperimentState(double time)
    : LearningPracticeExperimentState(time)
{
}

LearningPracticeListExperimentState::LearningPracticeListExperimentState(double time, int frame)
    : LearningPracticeExperimentState(time, frame)
{
}

LearningPracticeListExperimentState::LearningPracticeListExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
    : LearningPracticeExperimentState(time, frame, agents)
{
}

LearningPracticeListExperimentState::LearningPracticeListExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
                                                                   ALearningPracticeListPlayerState *playerState)
    : LearningPracticeExperimentState(time, frame, agents, playerState)
{
    this->currentTargets = playerState->GetCurrentTargets();
    this->isTargetVisited = playerState->GetIsTargetVisited();
    this->NumTargetsToCollect = playerState->GetNumTargetsToCollect();
	this->IsListMenuOpen = playerState->bIsDestinationsListVisible;
}

LearningPracticeListExperimentState::~LearningPracticeListExperimentState()
{
}

void LearningPracticeListExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
{
    WRITE_INDENTS(logFile, indentation);
    logFile << "<Num-To-Collect>" << NumTargetsToCollect << "</Num-To-Collect>" << endl;
    for (int i = 0; i < this->currentTargets.Num(); i++)
    {
        WRITE_INDENTS(logFile, indentation);
        logFile << "<Target" << WRITE_ATTRIBUTE_KV("id", this->currentTargets[i])
                << WRITE_ATTRIBUTE_KV("Visited", this->isTargetVisited[i])
                << "/>" << endl;
    }
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Is-List-Menu-Open>" << (IsListMenuOpen ? "true" : "false") << "</Is-List-Menu-Open>" << endl;
    NavigationExperimentState::WriteContents(logFile, indentation);
}
