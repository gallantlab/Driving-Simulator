// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "NavigationVehicleController.h"
#include "Util/NavigationParser.h"
#include "Game/CarlaGameModeBase.h"
#include "../Settings/CarlaSettings.h"

ANavigationVehicleController::ANavigationVehicleController(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
{
	destinationName = nullptr;
	destinations = ObjectInitializer.CreateDefaultSubobject<UDestinationParserComponent>(this, TEXT("DestinationParser"));
	randomStream = FRandomStream();
}


void ANavigationVehicleController::BeginPlay()
{
	Super::BeginPlay();

	// init random number generator
	const auto& CarlaSettings = gameInstance->GetCarlaSettings();
	randomStream.Initialize(CarlaSettings.SeedVehicles);
	randomDestinations = CarlaSettings.GetDestinationPickingMode() != EDestinationPickingMode::Predetermined;
}


FString ANavigationVehicleController::GetCurrentDestinationName() const
{
	if (!destinationName)
		return FString("");
	else return *destinationName;
}


void ANavigationVehicleController::ConfigureNextDestination()
{
	UE_LOG(LogFMRI, Log, TEXT("Generating next destination"));
	const auto& CarlaSettings = gameInstance->GetCarlaSettings();

	if (destination > -1)	// picking up from an interrupted trial
		SetCurrentDestination(destination);
	else
		PickNextDestination();

	destination = navigationPlayerState->currentDestination;

	destinationName = &(destinations->At(navigationPlayerState->currentDestination).GetName());
	UE_LOG(LogFMRI, Log, TEXT("Next destination is %s, index %d"), **destinationName, navigationPlayerState->currentDestination);
	UE_LOG(LogFMRI, Log, TEXT("Distance to new destination %d is %f, exclusion min %f meters, exclusion max start %f meters end %f meters"),
		   				 navigationPlayerState->currentDestination,
						 GetDistanceToDestination(),
						 destinations->EXCLUSION_MIN / 100.0,
						 destinations->EXCLUSION_MAX_START / 100.0,
						 destinations->EXCLUSION_MAX_END / 100.0);

	if (navigationPlayerState->secondsBeforeFirstDestination <= 0)
		RecordSecondsBeforeFirstDestination(timeSinceDemoStart);

	if (CarlaSettings.ShowNavigationInfo)
		destinations->At(navigationPlayerState->currentDestination).SetTriggerBoxVisibility(true);

	// cannot be lost at the beginning of a trial
	navigationPlayerState->isLost = false;
}


void ANavigationVehicleController::PickNextDestination(int minIndex, int maxIndex)
{
	if (randomDestinations)
	{
		SetCurrentDestination(destinations->GetNextDestination(GetPawn()));
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("Polling the game instance for the next destination"));
		SetCurrentDestination(gameInstance->GetCurrentNavigationDestination() % destinations->Num());	// mod to ensure safe indexing into array
	}
}

/// Get direction returns [-1, 1] in which negative is left and positive is right
/// value is the dot product of the view vector and the vector to the destination
/// Returns 0 for no destination to center arrow
/// \return
double ANavigationVehicleController::GetDirectionToDestination()
{
	if (navigationPlayerState->currentDestination < 0)
		return 0;
	FVector2D vectorToDestination = destinations->At(navigationPlayerState->currentDestination).GetDestination() - FVector2D(GetPawn()->GetActorLocation());
	vectorToDestination.Normalize();
	FVector2D viewVector = FVector2D(GetPawn()->GetBaseAimRotation().Vector());
	float multiplier = FVector2D::CrossProduct(viewVector, vectorToDestination) > 0 ? 0.5 : -0.5;
	return (1 - FVector2D::DotProduct(vectorToDestination, viewVector)) * multiplier;
}

double ANavigationVehicleController::GetDirectionToDestination(int destinationIndex)
{
	if (destinationIndex < 0 || destinationIndex >= destinations->Num())
		return 0;
	FVector2D vectorToDestination = destinations->At(destinationIndex).GetDestination() - FVector2D(GetPawn()->GetActorLocation());
	vectorToDestination /= vectorToDestination.Size();
	FVector2D viewVector = FVector2D(GetPawn()->GetBaseAimRotation().Vector());
	float multiplier = FVector2D::CrossProduct(viewVector, vectorToDestination) > 0 ? -0.5 : 0.5;
	return (1 - FVector2D::DotProduct(vectorToDestination, viewVector)) * multiplier;
}

void ANavigationVehicleController::GetDestinationParameters(FVector2D &egocentricVector, float &distance, float& direction)
{
#ifdef NAV
	if (navigationPlayerState->currentDestination < 0)
	{
		egocentricVector = FVector2D(0, 0);
		distance = direction = 0;
		return;
	}

	FVector2D destinationLocation = destinations->At(navigationPlayerState->currentDestination).GetDestination();
	FVector2D vectorToDestination = destinationLocation - FVector2D(GetPawn()->GetActorLocation());

	distance = vectorToDestination.Size();

	vectorToDestination.Normalize();
	FVector2D viewVector = FVector2D(GetPawn()->GetBaseAimRotation().Vector());
	float multiplier = FVector2D::CrossProduct(viewVector, vectorToDestination) > 0 ? 0.5 : -0.5;
	direction = (1 - FVector2D::DotProduct(vectorToDestination, viewVector)) * multiplier;

	// use vehicle and not current camera to calculate vector for minimap
	// since that rotation is based on vehicle heading
	egocentricVector = FVector2D(GetPawn()->GetRootComponent()->GetComponentRotation().UnrotateVector(FVector(vectorToDestination * distance, 0)));
#else
	egocentricVector = FVector2D(0, 0);
	distance = direction = 0;
#endif
}


void ANavigationVehicleController::Possess(APawn *pawn)
{
	Super::Possess(pawn);
	if (IsPossessingAVehicle())
	{
		navigationPlayerState = Cast<ANavigationPlayerState>(PlayerState);
		check(navigationPlayerState != nullptr);
	}
}

void ANavigationVehicleController::ExperimentTick(float dTime)
{
	if (navigationPlayerState->currentDestination < 0)		// no destination, waiting on one to be generated
	{
		if (secondsUntilNextDestination <= 0)					// is generating time
		{
			ConfigureNextDestination();
			SetDisplayText(GOTO + GetCurrentDestinationName(), EDisplayedPromptType::GoTo);	// display destination for two seconds
		}
		else secondsUntilNextDestination -= dTime;				// otherwise count down
	}
	else	// is currently navigating to a destination
	{
		if (CheckForArrival())		// close enough to destination and is stopped
		{
			OnSegmentEnd(12.0);
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
	}
}


void ANavigationVehicleController::OnSegmentEnd(float maxWait, bool isLost)
{
	// in case this gets called without a destination, otherwise segfault
	if (navigationPlayerState->currentDestination < 0)
		return;

	destination = -1;

	if (isLost)
	{
		UE_LOG(LogFMRI, Log, TEXT("Subject indicated that they are lost"));
		SetDisplayText(LOST, EDisplayedPromptType::Lost);								// display lost message for two seconds
		SetCurrentDestination(-2);
		navigationPlayerState->isLost = true;
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("Arrived at %s"), **destinationName);
		SetDisplayText(ARRIVEDAT + GetCurrentDestinationName(), EDisplayedPromptType::Arrive);					// display arrival message for two seconds
		SetCurrentDestination(-1);
	}
	secondsUntilNextDestination = maxWait > 4 ? randomStream.FRandRange(4.0f, maxWait) : 4;
	UE_LOG(LogFMRI, Log, TEXT("%f seconds until next destination"), secondsUntilNextDestination);
	if (!randomDestinations)
	{
		gameInstance->IncrementIndexInNavigationSequence();
		gameInstance->SaveSubjectStateFile();
	}
}


void ANavigationVehicleController::Tick(float dTime)
{
	Super::Tick(dTime);

	if (showArrival > 0)
		showArrival -= dTime;

	if (showDestination > 0)
		showDestination -= dTime;

	if (showLost > 0)
		showLost -= dTime;

	if (showHelp > 0)
		showHelp -= dTime;

	if (showHelp <= 0 && navigationPlayerState->showingHelp)
	{
		navigationPlayerState->showingHelp = false;
		ShowHelpInfoDelegate.ExecuteIfBound(false);
		showHelp = 0;
		UE_LOG(LogFMRI, Log, TEXT("Help showing ended. delegate is %sbound"), ShowHelpInfoDelegate.IsBound() ? TEXT("") : TEXT(" not"))
	}
}

bool ANavigationVehicleController::CheckForArrival()
{
	if (destinations->At(navigationPlayerState->currentDestination).IsProximal(GetPawn()) && abs(navigationPlayerState->GetForwardSpeed()) < 1)
	{
		if (gameInstance->GetCarlaSettings().ShowNavigationInfo)
			destinations->At(navigationPlayerState->currentDestination).SetTriggerBoxVisibility(false);
		return true;
	}
	return false;
}

void ANavigationVehicleController::TTLup()
{
	Super::TTLup();
	UE_LOG(LogFMRI, Log, TEXT("Current destination %d"), navigationPlayerState->currentDestination);
}

void ANavigationVehicleController::TTLdown()
{
	Super::TTLdown();
	// stuff for getting labels that Julie made
#ifdef RECORD_MARKED_LABELS
	for (TActorIterator<AActor> actorIterator(GetWorld()); actorIterator; ++actorIterator)
	{
		if (actorIterator->Tags.Num() > 0)
		{
			UE_LOG(LogFMRI, Log, TEXT("Actor %s %s"), *(actorIterator->GetName()), *(actorIterator->GetActorLocation().ToString()));
			for (FName tag : actorIterator->Tags)
				UE_LOG(LogFMRI, Log, TEXT("\tTag %s"), *(tag.ToString()));
		}
	}
	for (TActorIterator<ATriggerBox> triggerBoxIterator(GetWorld()); triggerBoxIterator; ++triggerBoxIterator)
	{
		if (triggerBoxIterator->Tags.Num() > 0)
		{
			UE_LOG(LogFMRI, Log, TEXT("Actor %s %s"), *(triggerBoxIterator->GetName()), *(triggerBoxIterator->GetActorLocation().ToString()));
			UE_LOG(LogFMRI, Log, TEXT("\tExtent %s"), *(triggerBoxIterator->GetCollisionComponent()->CalcBounds(triggerBoxIterator->GetCollisionComponent()->GetComponentTransform()).ToString()));
			for (FName tag : triggerBoxIterator->Tags)
				UE_LOG(LogFMRI, Log, TEXT("\tTag %s"), *(tag.ToString()));
		}
	}
#endif
}

bool ANavigationVehicleController::HasDestination() const
{
	return navigationPlayerState->currentDestination > -1;
}


const FString ANavigationVehicleController::GetProximalTarget()
{
	for (int i = 0; i < destinations->Num(); i++)
	{
		if (destinations->At(i).IsProximal(GetPawn()))
			return destinations->At(i).GetName();
	}
	return FString("None");
}


void ANavigationVehicleController::Reset()
{
	UE_LOG(LogFMRI, Log, TEXT("Run ended and the controller reset"));
	TTLsUntilFirstDestination = 5;

	for (int i = 0; i < destinations->Num(); i++)
		destinations->At(i).SetTriggerBoxVisibility(false);

	SetCurrentDestination(-1);
	navigationPlayerState->secondsBeforeFirstDestination = 0.0;
	secondsUntilNextDestination = 0;

	return;
}


void ANavigationVehicleController::ResetExperimentState()
{
	Super::ResetExperimentState();
	UE_LOG(LogFMRI, Log, TEXT("Navigation vehicle controller resetting experiment state"));
	TTLsUntilFirstDestination = 5;

	for (int i = 0; i < destinations->Num(); i++)
		destinations->At(i).SetTriggerBoxVisibility(false);

	SetCurrentDestination(-1);
	navigationPlayerState->secondsBeforeFirstDestination = 0.0;
	secondsUntilNextDestination = 0;

	return;
}


void ANavigationVehicleController::RecordSecondsBeforeFirstDestination(float seconds)
{
	UE_LOG(LogFMRI, Log, TEXT("First destination at time %f seconds"), seconds);
	navigationPlayerState->secondsBeforeFirstDestination = seconds;
}


void ANavigationVehicleController::RestartLevel()
{
	UE_LOG(LogFMRI, Log, TEXT("Game reset called"));
	gameInstance->SaveSubjectStateFile();

	Super::RestartLevel();
}


void ANavigationVehicleController::SetCurrentDestination(int newDestination, bool markerOff)
{
	// turn off the spinny cube - needs to be done for the end of MRI runs
	if (markerOff && (navigationPlayerState->currentDestination > -1))
		destinations->At(navigationPlayerState->currentDestination).SetTriggerBoxVisibility(false);

	UE_LOG(LogFMRI, Log, TEXT("Navigation Set destination value as %d"), newDestination);
	navigationPlayerState->currentDestination = newDestination;
	destinationName = newDestination > -1 ? &(destinations->At(navigationPlayerState->currentDestination).GetName()) : nullptr ;
}

FString ANavigationVehicleController::GetDisplayText() const
{
	if (NewDestination())
	{
		return GOTO + GetCurrentDestinationName();
	}
	else if (IsArrived())
	{
		return ARRIVEDAT + GetCurrentDestinationName();
	}
	else if (IsLost())
	{
		return LOST;
	}
	return EMPTY_STRING;
}

void ANavigationVehicleController::ReloadActiveDestinations()
{
	UE_LOG(LogFMRI, Log, TEXT("Reload active destinations called"))
	destinations->ParseActiveDestinations();
	GetGameInstance()->ReloadSettings();
	SetSecondsBeforeAISteering(GetGameInstance()->GetCarlaSettings().SecondsBeforeAISteering);
}


void ANavigationVehicleController::OpenSettingsMenu()
{
	ACarlaGameModeBase* gameMode = Cast<ACarlaGameModeBase>(GetWorld()->GetAuthGameMode());
	if (gameMode)
	{
		UE_LOG(LogFMRI, Log, TEXT("Opening settings menu"));
		gameMode->SettingsMenu();
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("Opening settings menu failed!"));
	}
}


void ANavigationVehicleController::ShowControls()
{
	ACarlaGameModeBase* gameMode = Cast<ACarlaGameModeBase>(GetWorld()->GetAuthGameMode());
	if (gameMode)
	{
		UE_LOG(LogFMRI, Log, TEXT("Opening settings menu to controls page"));
		gameMode->ShowControls();
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("Opening settings menu failed!"));
	}
}


void ANavigationVehicleController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (InputComponent)
	{
		// Opening settings menu
		InputComponent->BindAction("SettingsMenu", IE_Pressed, this, &ANavigationVehicleController::OpenSettingsMenu);
		InputComponent->BindAction("ShowControls", IE_Pressed, this, &ANavigationVehicleController::ShowControls);

		// Mechanism for the subject to indicate that they are lost
		InputComponent->BindAction("Button4", IE_Pressed, this, &ANavigationVehicleController::LostDown);
		InputComponent->BindAction("Button4", IE_Released, this, &ANavigationVehicleController::LostUp);

		// Temporarily displaying the HUD
		InputComponent->BindAction("Button3", IE_Pressed, this, &ANavigationVehicleController::NeedHelp);
	}
}


void ANavigationVehicleController::ShowHelp(double duration) {
	if (gameInstance->GetCarlaSettings().HelpButtonActive) {
		if (showHelp <= 0) {
			UE_LOG(LogFMRI, Log, TEXT("Need help pressed. delegate is %sbound"),
			       ShowHelpInfoDelegate.IsBound() ? TEXT("") : TEXT("not "))
			ShowHelpInfoDelegate.ExecuteIfBound(true);
		}
		showHelp = duration;
		navigationPlayerState->showingHelp = true;
	}
}




void ANavigationVehicleController::NeedHelp()
{
	ShowHelp(2.0);
}