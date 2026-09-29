// Copyright (c) 2026, Gallant Lab, University of California, Berkeley. This work is licensed under the terms of the BSD 3-Clause license. For a copy, see <https://opensource.org/licenses/BSD-3-Clause>.

#include "Carla.h"
#include "NamedTriggerBoxBase.h"


ANamedTriggerBoxBase::ANamedTriggerBoxBase(const FObjectInitializer& ObjectInitializer)
		: Super(ObjectInitializer)
{}

uint32 ANamedTriggerBoxBase::GetID()
{
	if (id == 0)
	{
		UE_LOG(LogFMRI, Log, TEXT("Hashing %s"), *Name);
		id = GetTypeHash(Name);
		UE_LOG(LogFMRI, Log, TEXT("ID %d"), id);
	}

	return id;
}

void ANamedTriggerBoxBase::BeginPlay()
{
	Super::BeginPlay();
	// because blueprints can't handle unsigned integers
	// and we don't want to use the negatives in the priority ordering
	// we just use negative numbers to mean lowest priority.
	if (Index < 0) Index = NOT_INDEXED;
}

void ANamedTriggerBoxBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ANamedTriggerBoxBase, Value);
}
