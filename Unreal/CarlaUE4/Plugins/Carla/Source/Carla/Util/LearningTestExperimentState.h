// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/LearningTestPlayerState.h"
#include "NavigationExperimentState.h"

/**
 * 
 */
class CARLA_API LearningTestExperimentState: public NavigationExperimentState
{
public:
	LearningTestExperimentState(double time);

	LearningTestExperimentState(double time, int frame);

	LearningTestExperimentState(double time, int frame, TArray<const UAgentComponent*> agents);

	LearningTestExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
						   ALearningTestPlayerState *playerState);

	~LearningTestExperimentState();


protected:
	virtual void WriteContents(std::ofstream &logFile, int &indentation) override;

private:
	int preConfidenceRating = -1;
	int postConfidenceRating = -1;
	FRotator relativeHeadingDirection = FRotator(0.f, 0.f, 0.f);
	FRotator absoluteHeadingDirection = FRotator(0.f, 0.f, 0.f);
};
