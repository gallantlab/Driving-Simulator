// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningTestExperimentState.h"

LearningTestExperimentState::LearningTestExperimentState(double time)
	: NavigationExperimentState(time)
{
}

LearningTestExperimentState::~LearningTestExperimentState()
{
}

LearningTestExperimentState::LearningTestExperimentState(double time, int frame)
	: NavigationExperimentState(time, frame)
{

}


LearningTestExperimentState::LearningTestExperimentState(double time, int frame, TArray<const UAgentComponent*> agents)
	: NavigationExperimentState(time, frame, agents)
{

}


LearningTestExperimentState::LearningTestExperimentState(double time, int frame, TArray<const UAgentComponent*> agents,
											   ALearningTestPlayerState *playerState)
	: NavigationExperimentState(time, frame, agents, playerState)
{
	preConfidenceRating = (int)playerState->GetPreConfidenceRating();
	postConfidenceRating = (int)playerState->GetPostConfidenceRating();
	relativeHeadingDirection = playerState->GetRelativeHeadingDirection();
	absoluteHeadingDirection = playerState->GetAbsoluteHeadingDirection();
}


void LearningTestExperimentState::WriteContents(std::ofstream &logFile, int &indentation)
{
//	UE_LOG(LogFMRI, Log, TEXT("LearningTest experiment state write content"));
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Learning-Test Pre Confidence=\"" << preConfidenceRating << "\"/>" << endl;
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Learning-Test Post Confidence=\"" << postConfidenceRating << "\"/>" << endl;
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Relative-Heading-Direction>" << relativeHeadingDirection.Pitch << ',' << relativeHeadingDirection.Yaw << ',' << relativeHeadingDirection.Roll << "</Relative-Heading-Direction>" << endl;
	WRITE_INDENTS(logFile, indentation);
	logFile << "<Absolute-Heading-Direction>" << absoluteHeadingDirection.Pitch << ',' << absoluteHeadingDirection.Yaw << ',' << absoluteHeadingDirection.Roll << "</Absolute-Heading-Direction>" << endl;

	NavigationExperimentState::WriteContents(logFile, indentation);
}