// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "MRIPlayerController.h"
#include "Traffic/SpeedZoneBase.h"
#include "Settings/CarlaSettings.h"
#include "Game/CarlaGameModeBase.h"

#define BOOL_TO_TEXT(x) x ? TEXT("yes") : TEXT("no")

AMRIPlayerController::AMRIPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	randomStream = FRandomStream();
}


void AMRIPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// try to attach self to the game instance to know when a recording is ended
	FindGameInstance();
	bShowMouseCursor = false;

	// eyetracking and demo recording stuff
	const auto& CarlaSettings = gameInstance->GetCarlaSettings();

	// as of 12/2020, this type does not occur in the scanner so no need to wait
	if (CarlaSettings.GetExperimentType() == EExperimentType::TrackedLearning)
		TTLsToExperimentStart = 1;
	if (CarlaSettings.GetExperimentType() == EExperimentType::LearningPractice)
		TTLsToExperimentStart = 1;

	autoEyetrack = CarlaSettings.AutoEyetrackingCalibration;
	if (!autoEyetrack)
		eyetrackingState = 2;
	autoTriggerDemo = CarlaSettings.AutoTriggerDemoRecording;

	UE_LOG(LogFMRI, Log, TEXT("MRI controller auto eyetrack %s auto demo record %s"), BOOL_TO_TEXT(autoEyetrack), BOOL_TO_TEXT(autoTriggerDemo));

	FindGameInstance();

	autoStopDemoLimit = CarlaSettings.SecondsToDemoStop;
	bAutoStopDemo = autoStopDemoLimit > 0;

	if (!gameInstance->HasResolutionBeenSet())
	{
		ConsoleCommand(*FString("r.setRes ").Append(CarlaSettings.Resolution));
		gameInstance->UpdateResolutionHasBeenSet();
	}

	// if auto-rendering has been specified, force game instance to enumerate replays
	// because that triggers the next auto rendering
	// and this will continue until all has been rendered because a new MRI player controller
	// is spawned after each demo replay finishes.
	if (CarlaSettings.RenderAll)
	{
		gameInstance->FindReplays();
		if (gameInstance->replayToRender.Len() > 0)
			gameInstance->DemoRenderFrames(gameInstance->replayToRender, CarlaSettings.RenderAll);
	}
}


void AMRIPlayerController::Possess(APawn *pawn)
{
	Super::Possess(pawn);
	if (IsPossessingAVehicle())
	{
		MRIPlayerState = Cast<AMRIPlayerState>(PlayerState);
		check(MRIPlayerState != nullptr);
	}
}


bool AMRIPlayerController::FindGameInstance()
{
	UE_LOG(LogFMRI, Log, TEXT("Trying to find game instance"));
	gameInstance = Cast<UCarlaGameInstance>(GetWorld()->GetGameInstance());
	if (gameInstance)
	{
		UE_LOG(LogFMRI, Log, TEXT("MRI controller found game instance"));
		return true;
	}
	UE_LOG(LogFMRI, Error, TEXT("MRI controller failed to find game instance"));
	return false;
}


void AMRIPlayerController::DespawnVehicle(EDespawnReason reason)
{
	UE_LOG(LogFMRI, Log, TEXT("Despawn called on subject and has no effect"));
}


void AMRIPlayerController::Tick(float dTime)
{
	Super::Tick(dTime);

	UpdateDemoState();
	if (currentDemoState == 1)	// count time since demo started if demo is recording
		timeSinceDemoStart += dTime;

	if (!TTLsToExperimentStart)
	{
		ExperimentTick(dTime);
		if (IsDemoEnded())	// reset exp data when demo recording is ended for this run
			ResetExperimentState();
	}

	if (displayTextTimeRemaining > 0)
	{
		displayTextTimeRemaining -= dTime;
		if (displayTextTimeRemaining <= 0)
		{
			ClearDisplayText();
		}
	}

	secondsWithoutTTL += dTime;
	if (bAutoStopDemo && !isFirstTTLInRun)
		if (secondsWithoutTTL > autoStopDemoLimit)
		{
			gameInstance->StopRecordingReplay();
			UE_LOG(LogFMRI, Log, TEXT("MRI player controller stopping demo after %.2f seconds without a TTL after %d TTLs"), secondsWithoutTTL, CarlaPlayerState->GetTotalTTLs());
		}

}

void AMRIPlayerController::ClearDisplayText()
{
	SetDisplayTextDelegate.ExecuteIfBound(EMPTY_STRING);
	SetDisplayTextColorDelegate.ExecuteIfBound(EDisplayTextColor::White);
	OnEndDisplayText();
}


void AMRIPlayerController::SetDisplayText(const FString& text, double duration, EDisplayedPromptType displayedPromptType, EDisplayTextColor color)
{
	SetDisplayTextDelegate.ExecuteIfBound(text);
	SetDisplayTextColorDelegate.ExecuteIfBound(color);
	MRIPlayerState->displayedPromptType = displayedPromptType;
	displayTextTimeRemaining = duration;
}


void AMRIPlayerController::SetDisplayText(const FString& text, EDisplayedPromptType displayedPromptType, double duration, EDisplayTextColor color)
{
	SetDisplayText(text, duration, displayedPromptType, color);
}


void AMRIPlayerController::OnEndDisplayText()
{
	MRIPlayerState->displayedPromptType = EDisplayedPromptType::None;
}

void AMRIPlayerController::UpdatePoints(int increment)
{
	Super::UpdatePoints(increment);
	SetDisplayPointsDelegate.ExecuteIfBound(CarlaPlayerState->GetPoints());
}



void AMRIPlayerController::TTLup()
{
	Super::TTLup();
	if (autoEyetrack && !(eyetrackingState))
	{
		ACarlaGameModeBase* gameMode = Cast<ACarlaGameModeBase>(GetWorld()->GetAuthGameMode());
		if (gameMode)
		{
			UE_LOG(LogFMRI, Log, TEXT("Auto starting eyetracking calibration"));
			gameMode->CalibrateEyetracking();
			eyetrackingState = 1;
		}
		else
		{
			UE_LOG(LogFMRI, Log, TEXT("Getting carla game mode failed!"));
		}
	}

	if (TTLsToExperimentStart && (eyetrackingState == 2)) TTLsToExperimentStart--;

	UE_LOG(LogFMRI, Log, TEXT("Current eyetracking state %d"), eyetrackingState);

	if (TTLsToExperimentStart)
		UE_LOG(LogFMRI, Log, TEXT("%d more TTLs experiment start"), TTLsToExperimentStart);

}


void AMRIPlayerController::TTLdown()
{
	Super::TTLdown();
	secondsWithoutTTL = 0.0;
	if (!gameInstance)
		FindGameInstance();
	if (!gameInstance)
		UE_LOG(LogFMRI, Error, TEXT("Game instance finding for TTL down failed"));
	FTimespan elapsed = FDateTime::Now() - gameInstance->demoStartTime;
	UE_LOG(LogFMRI, Log, TEXT("%02d:%02d:%02d.%03d since demo start"), elapsed.GetHours(), elapsed.GetMinutes(), elapsed.GetSeconds(), elapsed.GetFractionMilli());
	if (isFirstTTLInRun)
	{
		if (autoTriggerDemo)
		{
			if (!gameInstance)
			{
				UE_LOG(LogFMRI, Error, TEXT("No game instance; did not auto trigger demo start"));
			}
			else
			{
				if (!gameInstance->IsRecordingReplay())
				{
					gameInstance->StartRecordingReplay(FString(), FString());    // empty FStrings
					UE_LOG(LogFMRI, Log, TEXT("Demo recording auto-triggered by first TTL"));
					elapsed = FDateTime::Now() - gameInstance->demoStartTime;    // need to do this because otherwise elapsed will be calculated based on game init time (what demoStartTime is init to)
				}
			}
		}

		for (TActorIterator <ASpeedZoneBase> speedZone(GetWorld()); speedZone; ++speedZone)
		{
			if (!speedZone->bIsIntersection)
			{
				for (AActor* child : speedZone->Children)
				{
					if (Cast<ATriggerBox>(child) != nullptr)
					{
						UE_LOG(LogFMRI, Log, TEXT("Speedzone %s %s limit %:.2f"), *(speedZone->GetName()), *(speedZone->GetActorLocation().ToString()), speedZone->speedLimit);
						UE_LOG(LogFMRI, Log, TEXT("\tExtent %s"), *(Cast<ATriggerBox>(child)->GetCollisionComponent()->CalcBounds(Cast<ATriggerBox>(child)->GetCollisionComponent()->GetComponentTransform()).ToString()));
					}
				}
			}
		}

		SetSecondsToStartOfRun(elapsed.GetTotalSeconds());
		isFirstTTLInRun = false;
	}
}


void AMRIPlayerController::UpdateDemoState()
{
	if (!gameInstance)
		FindGameInstance();
	if (!gameInstance)
	{
		UE_LOG(LogFMRI, Error, TEXT("Game instance finding failed, demo state not updated"));
		return;
	}
	currentDemoState = gameInstance->GetDemoState();
}


void AMRIPlayerController::ResetExperimentState()
{
	lastDemoState = 0;
	currentDemoState = 0;
	timeSinceDemoStart = 0.0;
	eyetrackingState = autoEyetrack ? 0 : 2;
	isFirstTTLInRun = true;
	TTLsToExperimentStart = (gameInstance->GetCarlaSettings().GetExperimentType() == EExperimentType::TrackedLearning) ? 1 : 5;
	secondsWithoutTTL = 0;
	CarlaPlayerState->ResetExperimentState();
}


bool AMRIPlayerController::IsDemoEnded()
{
	switch (currentDemoState)
	{
		case 0:
			lastDemoState = 0;
			return false;
		case 1:
			lastDemoState = 1;
			return false;
		case 2:
			if (lastDemoState < 2)	// demo state switched into this status on last tick, need to do things
			{
				UE_LOG(LogFMRI, Log, TEXT("Demo stop detected"));
				lastDemoState = 2;
				return true;
			}
			else return false;
		default:
			UE_LOG(LogFMRI, Error, TEXT("Supposedly unreachable code reached"));
			return false;
	}
}


UCarlaGameInstance* AMRIPlayerController::GetGameInstance()
{
	if (!gameInstance) FindGameInstance();
	return gameInstance;
}