// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.
//
// Created by sjshim on 7/17/25.
//

#include "Carla.h"
#include "LearningPracticeExperimentState.h"

#include <iostream>
#include <fstream>

LearningPracticeExperimentState::LearningPracticeExperimentState(double time)
    : ForagingExperimentState(time)
{
}

LearningPracticeExperimentState::LearningPracticeExperimentState(double time, int frame)
    : ForagingExperimentState(time, frame)
{
}
LearningPracticeExperimentState::LearningPracticeExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
    : ForagingExperimentState(time, frame, agents)
{
}

LearningPracticeExperimentState::LearningPracticeExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
                                                                   ALearningPracticePlayerState *playerState)
    : ForagingExperimentState(time, frame, agents, playerState)
{
    this->currentTargets = playerState->GetCurrentTargets();
    this->isTargetVisited = playerState->GetIsTargetVisited();
    this->NumTargetsToCollect = playerState->GetNumTargetsToCollect();
}

LearningPracticeExperimentState::~LearningPracticeExperimentState()
{
}

void LearningPracticeExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
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

    NavigationExperimentState::WriteContents(logFile, indentation);
}
