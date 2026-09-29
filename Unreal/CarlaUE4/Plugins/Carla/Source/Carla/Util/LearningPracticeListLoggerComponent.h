// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Game/LearningPracticeListPlayerState.h"
#include "Util/LearningPracticeLoggerComponent.h"
#include "LearningPracticeListLoggerComponent.generated.h"

/**
 * 
 */
UCLASS()
class CARLA_API ULearningPracticeListLoggerComponent : public ULearningPracticeLoggerComponent
{
	GENERATED_BODY()

public:
	virtual void LogFrame() override;

protected:
	ALearningPracticeListPlayerState *learningPracticeListPlayerState;
	
};
