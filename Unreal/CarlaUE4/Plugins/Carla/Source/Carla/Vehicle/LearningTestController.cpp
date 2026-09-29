// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "LearningTestController.h"


void ALearningTestController::Possess(APawn *pawn)
{
	Super::Possess(pawn);
	if (IsPossessingAVehicle())
	{
		learningTestPlayerState = Cast<ALearningTestPlayerState>(PlayerState);
		check(learningTestPlayerState != nullptr);
	}
}

void ALearningTestController::ExperimentTick(float dTime) {
	switch (taskState)
	{
		case ETaskState::Intertrial:
			bAllowUserInput = false; // disable user input for driving
			if (navigationPlayerState->GetCurrentDestination() < 0)	// no destination, waiting on one to be generated
			{
				if (secondsUntilNextDestination <= 0)				// is generating time
				{
					ShowDestination();
					showDestinationTimer = 2.0f;	// display destination for two seconds
					taskState = ETaskState::ShowDestination;
				}
				else {
					secondsUntilNextDestination -= dTime;
				}
			}
			break;

		case ETaskState::ShowDestination:
			showDestinationTimer -= dTime;
			if (showDestinationTimer <= 0.0f) {
				taskState = ETaskState::HeadingDirectionQuery;
				SetUpQueryHeadingDirection();
				OnEndDisplayText();
			}
			break;


		default:
			if (CheckForArrival()) {
				taskState = ETaskState::Arrived;
				OnEndDisplayText();
			}
			if (lost)
			{
				lostDown -= dTime;
				if (lostDown <= 0)
				{
					lost = false;
					lostPressCount = 0;
				}
				else if (lostPressCount > 2)
				{
					lost = false;
					lostPressCount = 0;
					OnSegmentEnd(12.0, true);
				}
			}
			break;
	}
}

void ALearningTestController::ShowDestination() {
	bAllowUserInput = false; // disable user input for driving
	ConfigureNextDestination();
	SetDisplayText(GOTO + GetCurrentDestinationName(), 2, EDisplayedPromptType::GoTo);
}


void ALearningTestController::SetUpQueryHeadingDirection() {
	// Move camera up so that it doesn't show car
	OnBoardCamera->AddRelativeLocation(FVector(0.f, 0.f, 100.f));
}

void ALearningTestController::OnSegmentEnd(float maxWait, bool isLost)
{
	Super::OnSegmentEnd(maxWait, isLost);
	taskState = ETaskState::Intertrial;
	learningTestPlayerState->preconfidence = EConfidence::Undefined;
	learningTestPlayerState->postconfidence = EConfidence::Undefined;
}

void ALearningTestController::Reset()
{
	Super::Reset();
	taskState = ETaskState::Intertrial;
}


void ALearningTestController::ResetExperimentState()
{
	Super::ResetExperimentState();
	taskState = ETaskState::Intertrial;
}

bool ALearningTestController::CheckForArrival()
{
	if (navigationPlayerState->GetCurrentDestination() < 0)
		return false;

	if (bArriveButtonPressed) {
		bArriveButtonPressed = false;
		return true;
	}
	return false;
}

void ALearningTestController::OnEndDisplayText()
{
	switch (taskState)
	{
		case ETaskState::Start:
			SetDisplayText(QUERY_CONFIDENCE, 10, EDisplayedPromptType::QueryConfidence);
			taskState = ETaskState::StartConfidenceQuery;
			SetExperimentInputMode(EInputMode::RatingResponse);
			break;
		case ETaskState::Arrived:
			SetDisplayText(QUERY_CONFIDENCE, 10, EDisplayedPromptType::QueryConfidence);
			taskState = ETaskState::ArrivedConfidenceQuery;
			SetExperimentInputMode(EInputMode::RatingResponse);
			break;
		case ETaskState::HeadingDirectionQuery:
			SetDisplayText(QUERY_HEADING_DIRECTION, 2, EDisplayedPromptType::QueryHeadingDirection);
			SetExperimentInputMode(EInputMode::HeadingDirectionResponse);
			break;
		case ETaskState::StartConfidenceQuery:
			// this is only called to here if there is no response
			RecordConfidence(EConfidence::None);
			break;
		case ETaskState::ArrivedConfidenceQuery:
			// this is only called to here if there is no response
			RecordConfidence(EConfidence::None);
			break;
		case ETaskState::ShowPostConfidence:
			OnSegmentEnd(12, false);
			break;
		default:
			Super::OnEndDisplayText();
	}
}

void ALearningTestController::RecordConfidence(EConfidence confidence)
{
	switch (confidence)
	{
		case EConfidence::LOW:
			SetDisplayText(CONFIDENCE_1, EDisplayedPromptType::ConfidenceLow);
			break;
		case EConfidence::LOW_MED:
			SetDisplayText(CONFIDENCE_2, EDisplayedPromptType::ConfidenceLowMed);
			break;
		case EConfidence::MEDIUM:
			SetDisplayText(CONFIDENCE_3, EDisplayedPromptType::ConfidenceMed);
			break;
		case EConfidence::MED_HIGH:
			SetDisplayText(CONFIDENCE_4, EDisplayedPromptType::ConfidenceMedHigh);
			break;
		case EConfidence::HIGH:
			SetDisplayText(CONFIDENCE_5, EDisplayedPromptType::ConfidenceHigh);
			break;
		case EConfidence::None:
			SetDisplayText(CONFIDENCE_NO_INPUT, EDisplayedPromptType::ConfidenceNoResponse);
			break;
	}

	if (taskState == ETaskState::StartConfidenceQuery)
	{
		learningTestPlayerState->preconfidence = confidence;
		taskState = ETaskState::Navigation; // after confidence query, go back to navigation
	}
	else if (taskState == ETaskState::ArrivedConfidenceQuery)
	{
		learningTestPlayerState->postconfidence = confidence;
		taskState = ETaskState::ShowPostConfidence;
	}

	SetExperimentInputMode(EInputMode::Driving);
}

void ALearningTestController::SetupInputComponent() {
	Super::SetupInputComponent();
	if (InputComponent)
	{
		for (int32 i = InputComponent->GetNumActionBindings() - 1; i >= 0; --i)
		{
			const FInputActionBinding& Binding = InputComponent->GetActionBinding(i);
			if (Binding.ActionName == "Button3")
			{
				InputComponent->RemoveActionBinding(i);
			}
			if (Binding.ActionName == "Button4") {
				InputComponent->RemoveActionBinding(i);
			}
		}
		InputComponent->BindAction("Confirm", IE_Pressed, this, &ALearningTestController::ArriveButtonPress);
	}
}

void ALearningTestController::SetExperimentInputMode(EInputMode inputMode)
{
	InputComponent->ClearActionBindings();
	InputComponent->AxisBindings.Empty();

	if (inputMode == EInputMode::Driving) // double check
	{
		// SetupInputComponent does regular controls
		bAllowUserInput = true; // enable user input for driving
		this->SetupInputComponent();
	}
	else if (inputMode == EInputMode::RatingResponse) // EInputMode::RatingResponse
	{
		// bind rating controls
		bAllowUserInput = false; // disable user input for driving
		InputComponent->BindAction("Button1", IE_Pressed, this, &ALearningTestController::Button1Press);
		InputComponent->BindAction("Button2Press", IE_Pressed, this, &ALearningTestController::Button2Press);
		InputComponent->BindAction("Button3", IE_Pressed, this, &ALearningTestController::Button3Press);
		InputComponent->BindAction("Button4", IE_Pressed, this, &ALearningTestController::Button4Press);
		InputComponent->BindAction("Button6", IE_Pressed, this, &ALearningTestController::Button5Press);
	}
	else if (inputMode == EInputMode::HeadingDirectionResponse)
	{
		bAllowUserInput = false; // disable user input for driving
		InputComponent->BindAxis("MoveRight", this, &ALearningTestController::OnSteeringInput);
		InputComponent->BindAction("Confirm", IE_Pressed, this, &ALearningTestController::HeadingDirectionConfirmation);
	}
}

void ALearningTestController::OnSteeringInput(float Value) {
	OnBoardCamera->AddRelativeRotation(FRotator(0.f, Value, 0.f));
}


void ALearningTestController::ArriveButtonPress() {
	bArriveButtonPressed = true;
}

void ALearningTestController::HeadingDirectionConfirmation() {
	relativeHeadingDirection = OnBoardCamera->RelativeRotation;
	absoluteHeadingDirection = OnBoardCamera->GetComponentRotation();
	learningTestPlayerState->relativeHeadingDirection = relativeHeadingDirection;
	learningTestPlayerState->absoluteHeadingDirection = absoluteHeadingDirection;
	OnBoardCamera->SetRelativeRotation(FRotator(0.f, 0.f, 0.f));
	OnBoardCamera->SetRelativeLocation(FVector(160.f, 0.f, 120.f)); // reset camera position
	taskState = ETaskState::Start;
}

void ALearningTestController::Button1Press()
{
	RecordConfidence(EConfidence::LOW);
}

void ALearningTestController::Button2Press()
{
	RecordConfidence(EConfidence::LOW_MED);
}

void ALearningTestController::Button3Press()
{
	RecordConfidence(EConfidence::MEDIUM);
}

void ALearningTestController::Button4Press()
{
	RecordConfidence(EConfidence::MED_HIGH);
}

void ALearningTestController::Button5Press()
{
	RecordConfidence(EConfidence::HIGH);
}