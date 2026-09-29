// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "MRIPlayerState.h"

AMRIPlayerState::AMRIPlayerState()
	: Super()
{
	displayedPromptType = EDisplayedPromptType::None;
}

void AMRIPlayerState::Reset()
{
	Super::Reset();
	displayedPromptType = EDisplayedPromptType::None;
}

void AMRIPlayerState::ResetExperimentState()
{
	Super::ResetExperimentState();
	displayedPromptType = EDisplayedPromptType::None;
}

void AMRIPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);
	if ((PlayerState != nullptr) && (this != PlayerState))
	{
		AMRIPlayerState* Other = Cast<AMRIPlayerState>(PlayerState);
		if (Other != nullptr)
		{
			displayedPromptType = Other->displayedPromptType;
		}
	}
}

void AMRIPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AMRIPlayerState, displayedPromptType);
}