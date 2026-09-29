// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "Circuit.h"


// Sets default values
ACircuit::ACircuit()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
//	PrimaryActorTick.bCanEverTick = false;
}

void ACircuit::BeginPlay()
{
	Super::BeginPlay();

	FindTriggerBoxes();
}

void ACircuit::FindTriggerBoxes()
{
	// search for stretchy triggers marked with this circuit's name
	if (triggerBoxes.Num() < 1)
	{
		TArray<AActor *> Children = TArray<AActor *>();
		for (TActorIterator <ACircuitTrackBoxBase> trackBoxIterator(GetWorld()); trackBoxIterator; ++trackBoxIterator)
		{
			for (FName circuitTag : trackBoxIterator->Circuits)
			{
				if (circuitTag == CircuitName)
				{
					triggerBoxes.Add(*trackBoxIterator);
				}
			}
		}
	}
	UE_LOG(LogFMRI, Log, TEXT("Circuit %s has %d trigger boxes"), *(CircuitName.ToString()), triggerBoxes.Num());
}


void ACircuit::SetIsActive(bool state)
{
	UE_LOG(LogFMRI, Log, TEXT("Set %s %s"), *(CircuitName.ToString()), state ? TEXT("active") : TEXT("inactive"))
	if (state)
		FindTriggerBoxes();
	bIsActive = state;
}

EPlayerCircuitState ACircuit::Overlaps(const AActor * other)
{
	if (triggerBoxes.Num() < 1)
		return EPlayerCircuitState::NoCourse;
	bool hasOverlap = false;
	double dotSum = 0;
	for (int i = 0; i < triggerBoxes.Num(); i++)
		if (triggerBoxes[i]->TriggerBoxOverlaps(other))
		{
			hasOverlap = true;
			dotSum += FVector::DotProduct(triggerBoxes[i]->Direction, other->GetActorForwardVector());
		}
	if (hasOverlap)
	{
		if (dotSum > 0)
			return EPlayerCircuitState::OnCourse;
		else return EPlayerCircuitState::WrongDirection;
	}
	return EPlayerCircuitState::OffCourse;
}