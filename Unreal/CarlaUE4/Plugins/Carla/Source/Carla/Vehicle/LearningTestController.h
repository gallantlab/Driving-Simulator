// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#pragma once

#include "CoreMinimal.h"
#include "Vehicle/NavigationVehicleController.h"
#include "Game/LearningTestPlayerState.h"
#include "LearningTestController.generated.h"

class UCameraComponent;
class USpringArmComponent;

const FString QUERY_CONFIDENCE = FString("rate Confidence 1-5");
const FString CONFIDENCE_1 = FString("Confidence 1");
const FString CONFIDENCE_2 = FString("Confidence 2");
const FString CONFIDENCE_3 = FString("Confidence 3");
const FString CONFIDENCE_4 = FString("Confidence 4");
const FString CONFIDENCE_5 = FString("Confidence 5");
const FString CONFIDENCE_NO_INPUT = FString("No rating");
const FString QUERY_HEADING_DIRECTION = FString("Face direction of destination");

enum class EInputMode: uint8
{
	Driving,
	RatingResponse,
	HeadingDirectionResponse,
};

enum class ETaskState: uint8
{
	Start,
	ShowDestination,
	StartConfidenceQuery,
	HeadingDirectionQuery,
	Navigation,
	Arrived,
	ArrivedConfidenceQuery,
	Intertrial,
	ShowPostConfidence
};

/**
 * Controller class used in the learning test sessions
 */
UCLASS()
class CARLA_API ALearningTestController : public ANavigationVehicleController
{
	GENERATED_BODY()

	virtual void ExperimentTick(float dTime) override;

	virtual void Possess(APawn *pawn) override;

	virtual void ShowDestination();

	virtual void SetUpQueryHeadingDirection();

	virtual void OnSegmentEnd(float maxWait, bool isLost = false) override;

	// reset state at end of a single run
	virtual void Reset() override;

	virtual void ResetExperimentState() override;

protected:
	ALearningTestPlayerState* learningTestPlayerState;

	virtual bool CheckForArrival() override;

	virtual void OnEndDisplayText() override;

	void ArriveButtonPress();

	void HeadingDirectionConfirmation();

	void Button1Press();

	void Button2Press();

	void Button3Press();

	void Button4Press();

	void Button5Press();

	void RecordConfidence(EConfidence confidence);

	void SetupInputComponent() override;

	void OnSteeringInput(float Value);

private:
	bool bArriveButtonPressed = false;

	EInputMode currentInputMode = EInputMode::Driving;

	void SetExperimentInputMode(EInputMode inputMode);

	ETaskState taskState = ETaskState::Intertrial;

	float showDestinationTimer = 0.0f;

	FRotator relativeHeadingDirection;

	FRotator absoluteHeadingDirection;
};
