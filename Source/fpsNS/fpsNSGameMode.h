// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "fpsNSGameMode.generated.h"

UENUM(BlueprintType)
enum class ETeam : uint8
{
	BLUE_TEAM,
	RED_TEAM
};

UCLASS(minimalapi)
class AfpsNSGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AfpsNSGameMode();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void Respawn(class AfpsNSCharacter* Character);
	void Spawn(class AfpsNSCharacter* Character);

private:
	TArray<class AfpsNSCharacter*> RedTeam;
	TArray<class AfpsNSCharacter*> BlueTeam;

	TArray<class ANSSpawnPoint*> RedSpawns;
	TArray<class ANSSpawnPoint*> BlueSpawns;
	TArray<class AfpsNSCharacter*> ToBeSpawned;

	bool bGameStarted;
	static bool bInGameMenu;
};



