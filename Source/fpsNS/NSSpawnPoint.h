// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "fpsNSGameMode.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Actor.h"
#include "NSSpawnPoint.generated.h"

UCLASS()
class FPSNS_API ANSSpawnPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANSSpawnPoint();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION()
	void ActorBeginOverlaps(AActor* OverlappedActor, AActor* OtherActor);

	UFUNCTION()
	void ActorEndOverlaps(AActor* OverlappedActor, AActor* OtherActor);

	bool GetBlocked()
	{
		return OverlappingActors.Num() != 0;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	ETeam Team;

	bool isTaken = false;

protected:
	UCapsuleComponent* SpawnCapsule;
	TArray<class AActor*> OverlappingActors;

};
