// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "Util/NamedTriggerBoxBase.h"
#include "DestinationParserComponent.h"
#include "NavigationParser.h"
#include "NeighborhoodBoxBase.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/CarlaSettings.h"


// Sets default values for this component's properties
UDestinationParserComponent::UDestinationParserComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	destinations = TArray<NavigationDestination>();

	activeDestinations = TArray<int>();

	isActive = TArray<bool>();

	hasBeenVisited = TArray<bool>();

	none = NavigationDestination();

	randomStream = FRandomStream();

	allNeighborhoods = TArray<ANeighborhoodBoxBase*>();

	activeNeighborhoods = TArray<ANeighborhoodBoxBase*>();
}

void UDestinationParserComponent::BeginPlay()
{
	GetDestinationsFromMap();
	GetNeighborhoodsFromMap();
	const UCarlaSettings& settings = Cast<UCarlaGameInstance>(GetOwner()->GetGameInstance())->GetCarlaSettings();
	ParseActiveDestinations(settings.DestinationsFile);

	EXCLUSION_MIN = settings.ExclusionMin;
	EXCLUSION_MAX_START = settings.ExclusionMaxStart;
	EXCLUSION_MAX_END = settings.ExclusionMaxEnd;
	EXCLUSION_MIN_PROBABILITY = settings.ExclusionMinProbability;

	lowValuePoints = settings.LowValuePoints;
	medValuePoints = settings.MedValuePoints;
	highValuePoints = settings.HighValuePoints;

	destinationPickingMode = settings.GetDestinationPickingMode();

	randomStream.Initialize(settings.SeedVehicles);
}

int UDestinationParserComponent::GetDestinationsFromMap()
{
	// looks for named trigger boxes
	UE_LOG(LogFMRI, Log, TEXT("Looking for things in the map"));
	ANamedTriggerBoxBase *triggerBox;
	int totalDestinations = 0;
	for (TActorIterator<ANamedTriggerBoxBase> triggerBoxIterator(GetWorld()); triggerBoxIterator; ++triggerBoxIterator)
	{
		totalDestinations++;
		triggerBox = *triggerBoxIterator;
#ifdef LOG_TRIGGER_BOXES
		UE_LOG(LogFMRI, Log, TEXT("Trigger box %s"), *(triggerBox->Name));
		UE_LOG(LogFMRI, Log, TEXT("\tLocation %s Rotation %s"), *(triggerBox->GetActorLocation().ToString()), *(triggerBox->GetActorRotation().ToString()));
		UE_LOG(LogFMRI, Log, TEXT("\tExtent %s"), *(Cast<UBoxComponent>(triggerBoxIterator->GetCollisionComponent())->GetScaledBoxExtent().ToString()));
#endif
		if (triggerBox->Active)
		{
			destinations.Add(NavigationDestination(triggerBox));
			hasBeenVisited.Add(false);
		}
	}
	destinations.Sort();
	UE_LOG(LogFMRI, Log, TEXT("%d total location entries from the map"), totalDestinations);
	UE_LOG(LogFMRI, Log, TEXT("%d active location entries from the map"), destinations.Num());
	isIndexed = 0;
	int thisIndex;
	for (int i = 0; i < destinations.Num(); i++)
	{
		UE_LOG(LogFMRI, Log, TEXT("Destination %d: %s, id %u"), i, *(destinations[i].GetName()), destinations[i].GetID());
		thisIndex = destinations[i].GetIndex();
		if (thisIndex != NOT_INDEXED)
		{
			isIndexed++;
			if (thisIndex < minIndex) minIndex = thisIndex;
			if (thisIndex > maxIndex) maxIndex = thisIndex;
		}
	}
	if (isIndexed)
	{
		UE_LOG(LogFMRI, Log, TEXT("There are %d indexed locations, with min index %d and max index %d"), isIndexed, minIndex, maxIndex);
	}
	else
	{
		UE_LOG(LogFMRI, Log, TEXT("There are no explicitly indexed locations"));
	}
	return destinations.Num();
}


void UDestinationParserComponent::GetNeighborhoodsFromMap()
{
	for (TActorIterator<ANeighborhoodBoxBase> neighborhoodIterator(GetWorld()); neighborhoodIterator; ++neighborhoodIterator)
		allNeighborhoods.Add(*neighborhoodIterator);
	UE_LOG(LogFMRI, Log, TEXT("Found %d neighborhood bounding boxes"), allNeighborhoods.Num());
}


void UDestinationParserComponent::UpdateActiveNeighborhoods()
{
	UE_LOG(LogFMRI, Log, TEXT("Updating active neighborhoods list"))
	TMap<EDestinationSet, bool> &activeSets = Cast<UCarlaGameInstance>(GetOwner()->GetGameInstance())->GetCarlaSettings().ActiveDestinationSets;
	for (int i = 0; i < (int)EDestinationSet::Unknown - 1; i++)
	{
		UE_LOG(LogFMRI, Log, TEXT("%s is %sactive"), *UDestinationSet::ToString((EDestinationSet)i), activeSets[(EDestinationSet)i] ? TEXT("") : TEXT("not "));
	}

	activeNeighborhoods.Empty();
	for (ANeighborhoodBoxBase* box : allNeighborhoods)
		if (activeSets[box->neighborhood])
			activeNeighborhoods.Add(box);
	UE_LOG(LogFMRI, Log, TEXT("Found %d active neighborhood bounding boxes"), activeNeighborhoods.Num());
}

bool UDestinationParserComponent::IsActorInActiveNeighborhoods(AActor *actor) const
{
	for (ANeighborhoodBoxBase* box : activeNeighborhoods)
		if (box->IsOverlappingActor(actor))
			return true;
	return false;
}


// populating the isActive array also happens here
// because this method should be called in the parent controller's PostInitializeComponents
// which sould happen after beginPlay, so after this objects has parsed destinations
// from the map
int UDestinationParserComponent::ParseActiveDestinations(const FString &activeDestinationsFile)
{
	if (!activeDestinationsFile.Equals(TEXT("all")))
	{
		return ParseActiveDestinations();
	}
	else
	{
		if (isActive.Num() > 0)
		{
			UE_LOG(LogFMRI, Log, TEXT("Emptying isActive array before parsing active destinations"));
			isActive.Empty();
		}
		for (int i = 0; i < destinations.Num(); i++)
			isActive.Add(true);
		UE_LOG(LogFMRI, Log, TEXT("All destinations active"));

		UpdateActiveNeighborhoods();
		return destinations.Num();
	}
}

int UDestinationParserComponent::ParseActiveDestinations()
{
	if (isActive.Num() > 0)
	{
		UE_LOG(LogFMRI, Log, TEXT("Emptying isActive array before parsing active destinations"));
		isActive.Empty();
	}

	NavigationParser::ParseActiveDestinations(destinations, Cast<UCarlaGameInstance>(GetOwner()->GetGameInstance())->GetCarlaSettings().ActiveDestinationSets, this->activeDestinations);
	for (int i = 0; i < destinations.Num(); i++)
		isActive.Add(activeDestinations.Contains(destinations[i].GetID()));

	for (int i = 0; i < destinations.Num(); i++)
	{
		UE_LOG(LogFMRI, Log, TEXT("Destination %d: %s, id %u, %s active"), i, *(destinations[i].GetName()), destinations[i].GetID(), isActive[i] ? TEXT("is") : TEXT("isn't"));
	}

	UpdateActiveNeighborhoods();

	return activeDestinations.Num();
}


int UDestinationParserComponent::GetNumberOfActiveDestinations() const
{
	int out = 0;
	for (int i = 0; i < hasBeenVisited.Num(); i++)
		if (isActive[i])
			out++;
	return out;
}


int UDestinationParserComponent::GetNumberOfVisitedDestinations(bool activeOnly) const
{
	int out = 0;
	for (int i = 0; i < hasBeenVisited.Num(); i++)
		if (hasBeenVisited[i] && (activeOnly ? isActive[i] : true))
			out++;
	return out;
}

void UDestinationParserComponent::ClearVisitedDestinations()
{
	for (int i = 0; i < hasBeenVisited.Num(); i++)
		hasBeenVisited[i] = false;
}

#define GET_ABS_PATH(fileName) FPlatformFileManager::Get().GetPlatformFile().ConvertToAbsolutePathForExternalAppForWrite(*FPaths::Combine(FPaths::ProjectSavedDir(), fileName));

bool UDestinationParserComponent::SaveHasBeenVisited(const FString& fileName) const
{
	FString absFileName = GET_ABS_PATH(fileName);

	// because FFileHelper only writes uint8 and string arrays, we convert the visited locations to a uint8 explicitly
	// and also because of the way things are read back in, we insert a count of the number of entries at the beginning
	TArray<uint8> outArray;
	outArray.Add(hasBeenVisited.Num());
	for (int i = 0 ; i < hasBeenVisited.Num(); i++)
	{
		outArray.Add(hasBeenVisited[i] ? 1 : 0);
	}

	return FFileHelper::SaveArrayToFile(outArray, *absFileName);
}

bool UDestinationParserComponent::LoadHasBeenVisited(const FString& fileName)
{
	FString absFileName = GET_ABS_PATH(fileName);

	if (!FPaths::FileExists(absFileName))
		return false;

	TArray<uint8> inArray;

	if (FFileHelper::LoadFileToArray(inArray, *absFileName))
	{
		if (inArray.Num() <= inArray[0])
			return false;

		hasBeenVisited.Empty();
		for (int i = 1; i <= inArray[0]; i++)
			hasBeenVisited.Add(inArray[i] > 0);

		return true;
	}

	return false;
}

double UDestinationParserComponent::GetDistanceToDestination(AActor *actor, int destinationIndex) const
{
	if (destinationIndex < 0 || destinationIndex >= destinations.Num())
		return -1;
	FVector2D destinationLocation = destinations[destinationIndex].GetDestination();
	return FVector2D::Distance(destinationLocation, FVector2D(actor->GetActorLocation()));
}


int UDestinationParserComponent::PickBiasedRandomDestination(AActor *actor) const
{
	// in the case that all generated destinations are beyond the far exclusion, give the closest after a few tries
	int nextDestination = randomStream.RandHelper(destinations.Num());

	while(!isActive[nextDestination])
		nextDestination = randomStream.RandHelper(destinations.Num());

	double distanceToNextDestination = GetDistanceToDestination(actor, nextDestination);

	int closestDestination = nextDestination;
	for (int i = minIndex; i <= maxIndex; i++)
	{
		UE_LOG(LogFMRI, Log, TEXT("Destination %d distance %f"), nextDestination, distanceToNextDestination);
		// acceptable distance!
		if (distanceToNextDestination > EXCLUSION_MIN && distanceToNextDestination < EXCLUSION_MAX_START)
		{
			closestDestination = nextDestination;
			break;
		}
		// has a chance of being accepted
		if (distanceToNextDestination > EXCLUSION_MAX_START && distanceToNextDestination < EXCLUSION_MAX_END)
		{
			if (randomStream.FRand() < (distanceToNextDestination - EXCLUSION_MAX_START) / (EXCLUSION_MAX_END - EXCLUSION_MAX_START) * (1 - EXCLUSION_MIN_PROBABILITY) + EXCLUSION_MIN_PROBABILITY)
			{
				UE_LOG(LogFMRI, Log, TEXT("Location randomly accepted"));
				closestDestination = nextDestination;
				break;
			}
		}

		// generate a new one
		nextDestination = randomStream.RandHelper(destinations.Num());
		while(!isActive[nextDestination])
			nextDestination = randomStream.RandHelper(destinations.Num());
		distanceToNextDestination = GetDistanceToDestination(actor, nextDestination);
		if (distanceToNextDestination < GetDistanceToDestination(actor, closestDestination))
			closestDestination = nextDestination;
	}

	return closestDestination;
}

int UDestinationParserComponent::PickPurePermutedDestination(AActor *actor) const
{
	int nextDestination;
	do
	{
		nextDestination = randomStream.RandHelper(destinations.Num());
	} while (hasBeenVisited[nextDestination] || !isActive[nextDestination]);
	return nextDestination;
}

int UDestinationParserComponent::PickBiasedPermutedDestination(AActor *actor) const
{
	return -1;
}

bool UDestinationParserComponent::SetHasBeenVisited(int index, bool beenVisited)
{
	if (index < 0 || index >= hasBeenVisited.Num())
		return false;
	hasBeenVisited[index] = beenVisited;

	// reset if all has been visited
	bool allVisited = true;
	for (int i = 0; i < hasBeenVisited.Num(); i++)
	{
		if (!hasBeenVisited[i])
		{
			allVisited = false;
			break;
		}
	}
	if (allVisited)
	{
		for (int i = 0; i < hasBeenVisited.Num(); i++)
			hasBeenVisited[i] = false;
	}

	return true;
}

int UDestinationParserComponent::GetNextDestination(AActor *actor) const
{
	switch (destinationPickingMode)
	{
		case EDestinationPickingMode::BiasedRandom:
			return PickBiasedRandomDestination(actor);
		case EDestinationPickingMode::PurePermuted:
			return PickPurePermutedDestination(actor);
		case EDestinationPickingMode::BiasedPermuted:
			return PickBiasedPermutedDestination(actor);
		default:
			return PickPureRandomDestination(actor);
	}
}

void UDestinationParserComponent::RandomizeTargetValues()
{
	for (int i = 0; i < this->Num(); i++)
	{
		// the probability of spawning each kind of item is inversely proportional
		// its value
		double value = randomStream.GetFraction() * 7;
		if (value < 4)
			destinations[i].SetTargetValue(ETargetPointValue::Low);
		else if (value < 6)
			destinations[i].SetTargetValue(ETargetPointValue::Medium);
		else
			destinations[i].SetTargetValue(ETargetPointValue::High);
	}
}

int UDestinationParserComponent::GetTargetPointsValue(int target) const
{
	switch (destinations[target].GetTargetValue())
	{
	case ETargetPointValue::Low:
		return lowValuePoints;
	case ETargetPointValue::Medium:
		return medValuePoints;
	case ETargetPointValue::High:
		return highValuePoints;

	case ETargetPointValue::None:
	default:
		return 0;
	}
}
