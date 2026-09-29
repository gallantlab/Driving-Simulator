// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningTestPlayerState.h"


ALearningTestPlayerState::ALearningTestPlayerState()
	: Super()
{
	preconfidence = EConfidence::Undefined;
	postconfidence = EConfidence::Undefined;
	relativeHeadingDirection = FRotator(0.0f, 0.0f, 0.0f);
	absoluteHeadingDirection = FRotator(0.0f, 0.0f, 0.0f);
}

void ALearningTestPlayerState::Reset()
{
	Super::Reset();
	preconfidence = EConfidence::Undefined;
	postconfidence = EConfidence::Undefined;
	relativeHeadingDirection = FRotator(0.0f, 0.0f, 0.0f);
	absoluteHeadingDirection = FRotator(0.0f, 0.0f, 0.0f);
}

void ALearningTestPlayerState::ResetExperimentState()
{
	Super::ResetExperimentState();
	preconfidence = EConfidence::Undefined;
	postconfidence = EConfidence::Undefined;
	relativeHeadingDirection = FRotator(0.0f, 0.0f, 0.0f);
	absoluteHeadingDirection = FRotator(0.0f, 0.0f, 0.0f);
}

void ALearningTestPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if ((PlayerState != nullptr) && (this != PlayerState))
	{
		ALearningTestPlayerState* Other = Cast<ALearningTestPlayerState>(PlayerState);
		if (Other != nullptr)
		{
			preconfidence = Other->preconfidence;
			postconfidence = Other->postconfidence;
			relativeHeadingDirection = Other->relativeHeadingDirection;
			absoluteHeadingDirection = Other->absoluteHeadingDirection;
		}
	}
}

void ALearningTestPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ALearningTestPlayerState, preconfidence);
	DOREPLIFETIME(ALearningTestPlayerState, postconfidence);
	DOREPLIFETIME(ALearningTestPlayerState, relativeHeadingDirection);
	DOREPLIFETIME(ALearningTestPlayerState, absoluteHeadingDirection);
}