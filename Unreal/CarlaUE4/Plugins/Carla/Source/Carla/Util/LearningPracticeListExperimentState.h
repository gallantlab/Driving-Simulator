// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.
//
// Created by sjshim on 7/17/25.
//

#pragma once

#include "CoreMinimal.h"
#include "LearningPracticeExperimentState.h"
#include "Game/LearningPracticeListPlayerState.h"



class CARLA_API LearningPracticeListExperimentState : public LearningPracticeExperimentState
{
public:
    LearningPracticeListExperimentState(double time);

    LearningPracticeListExperimentState(double time, int frame);

    LearningPracticeListExperimentState(double time, int frame, TArray<const UAgentComponent*> agents);

    LearningPracticeListExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
                                    ALearningPracticeListPlayerState *playerState);

    ~LearningPracticeListExperimentState();

    TArray<int> GetCurrentTargets() const
    {
        return currentTargets;
    }
    TArray<bool> GetIsTargetVisited() const
    {
        return isTargetVisited;
    }
    int GetNumTargetsToCollect() const
    {
        return NumTargetsToCollect;
    }
protected:
    virtual void WriteContents(std::ofstream &logFile, int &indentation) override;

private:
    TArray<int> currentTargets;
    TArray<bool> isTargetVisited;
    int NumTargetsToCollect;
	bool IsListMenuOpen;
};