// Copyright Epic Games, Inc. All Rights Reserved.

#include "fpsNSGameMode.h"
#include "fpsNS.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "fpsNSHUD.h"
#include "fpsNSCharacter.h"
#include "NSPlayerState.h"
#include "NSSpawnPoint.h"
#include "NSGameStateBase.h"
#include "UObject/ConstructorHelpers.h"

bool AfpsNSGameMode::bInGameMenu = true;

AfpsNSGameMode::AfpsNSGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AfpsNSHUD::StaticClass();
	PlayerStateClass = ANSPlayerState::StaticClass();

	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	GameStateClass = ANSGameStateBase::StaticClass();
}

void AfpsNSGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (GetLocalRole() == ROLE_Authority)
	{
		Cast<ANSGameStateBase>(GameState)->bInMenu = bInGameMenu;

		for (TActorIterator<ANSSpawnPoint> Iter(GetWorld()); Iter; ++Iter)
		{
			if ((*Iter)->Team == ETeam::RED_TEAM)
			{
				RedSpawns.Add(*Iter);
			}
			else
			{
				BlueSpawns.Add(*Iter);
			}
		}

		// ���� ����
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();
		if (thisCont)
		{
			AfpsNSCharacter* thisChar = Cast<AfpsNSCharacter>(thisCont->GetPawn());
			thisChar->SetTeam(ETeam::BLUE_TEAM);
			BlueTeam.Add(thisChar);
			Spawn(thisChar);
		}
	}
}

void AfpsNSGameMode::Tick(float DeltaSeconds)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		APlayerController* thisCont = GetWorld()->GetFirstPlayerController();

		if (ToBeSpawned.Num() != 0)
		{
			for (auto charToSpawn : ToBeSpawned)
			{
				Spawn(charToSpawn);
			}
		}

		if (thisCont != nullptr && thisCont->IsInputKeyDown(EKeys::R))
		{
			bInGameMenu = false;
			GetWorld()->ServerTravel(L"/Game/FirstPersonCPP/Maps/FirstPersonExampleMap?Listen");

			Cast<ANSGameStateBase>(GameState)->bInMenu = bInGameMenu;
		}
	}
}

void AfpsNSGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	AfpsNSCharacter* Teamless = Cast<AfpsNSCharacter>(NewPlayer->GetPawn());
	ANSPlayerState* NPlayerState = Cast<ANSPlayerState>(NewPlayer->PlayerState);

	if (Teamless != nullptr && NPlayerState != nullptr)
	{
		Teamless->SetNSPlayerState(NPlayerState);
	}

	// �� ���� �� ����
	if (GetLocalRole() == ROLE_Authority && Teamless != nullptr)
	{
		if (BlueTeam.Num() > RedTeam.Num())
		{
			RedTeam.Add(Teamless);
			NPlayerState->Team = ETeam::RED_TEAM;
		}
		else if (BlueTeam.Num() < RedTeam.Num())
		{
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}
		else // ���� ����
		{
			BlueTeam.Add(Teamless);
			NPlayerState->Team = ETeam::BLUE_TEAM;
		}

		Teamless->CurrentTeam = NPlayerState->Team;
		Teamless->SetTeam(NPlayerState->Team);
		Spawn(Teamless);
	}
}

void AfpsNSGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason == EEndPlayReason::Quit || EndPlayReason == EEndPlayReason::EndPlayInEditor)
	{
		bInGameMenu = true;
	}
}

void AfpsNSGameMode::Respawn(AfpsNSCharacter* Character)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		AController* thisPC = Character->GetController();
		Character->DetachFromControllerPendingDestroy();

		AfpsNSCharacter* newChar = Cast<AfpsNSCharacter>(GetWorld()->SpawnActor(DefaultPawnClass));

		if (newChar)
		{
			thisPC->Possess(newChar);
			ANSPlayerState* thisPS = Cast<ANSPlayerState>(newChar->GetController()->PlayerState);

			newChar->CurrentTeam = thisPS->Team;
			newChar->SetNSPlayerState(thisPS);

			Spawn(newChar);

			newChar->SetTeam(newChar->GetNSPlayerState()->Team);
		}
	}
}

void AfpsNSGameMode::Spawn(AfpsNSCharacter* Character)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// ��ϵ��� ���� ���� ���� ã��
		ANSSpawnPoint* thisSpawn = nullptr;
		TArray<ANSSpawnPoint*>* targetTeam = nullptr;

		if (Character->CurrentTeam == ETeam::BLUE_TEAM)
		{
			targetTeam = &BlueSpawns;
		}
		else
		{
			targetTeam = &RedSpawns;
		}

		if (ToBeSpawned.Find(Character) == INDEX_NONE)
		{
			ToBeSpawned.Add(Character);
		}

		for (auto Spawn : (*targetTeam))
		{
			UE_LOG(LogTemp, Log, TEXT("Spawn Point = %s is Blocked = %s"), *Spawn->GetFName().ToString(), Spawn->GetBlocked() ? TEXT("true") : TEXT("false"));
			TSet<AActor*> actors;
			Spawn->GetOverlappingActors(actors);
			for (auto overlapActor : actors)
			{
				UE_LOG(LogTemp, Log, TEXT("Spawn Point OverlapActor = %s"), *overlapActor->GetFName().ToString());
			}

			if (actors.Num() == 0)
			//if(Spawn->isTaken == false)
			{
				// ���� ť ��ġ���� ����
				if (ToBeSpawned.Find(Character) != INDEX_NONE)
				{
					ToBeSpawned.Remove(Character);
				}

				// �׷��� ������ ���� ��ġ ����
				Character->SetActorLocation(Spawn->GetActorLocation());
				Spawn->UpdateOverlaps();
				//Spawn->isTaken = true;
				return;
			}
		}
	}
}
