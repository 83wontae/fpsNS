// Copyright Epic Games, Inc. All Rights Reserved.

#include "fpsNSCharacter.h"
#include "fpsNSProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "GameFramework/InputSettings.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "MotionControllerComponent.h"
#include "XRMotionControllerBase.h" // for FXRMotionControllerBase::RightHandSourceId

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// AfpsNSCharacter

AfpsNSCharacter::AfpsNSCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	FP_Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Mesh"));
	FP_Mesh->SetOnlyOwnerSee(true);
	FP_Mesh->SetupAttachment(FirstPersonCameraComponent);
	FP_Mesh->bCastDynamicShadow = false;
	FP_Mesh->CastShadow = false;
	FP_Mesh->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	FP_Mesh->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// otherwise won't be visible in the multiplayer
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	FP_Gun->SetupAttachment(FP_Mesh, TEXT("GripPoint"));
	//FP_Gun->SetupAttachment(RootComponent);

	//FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	//FP_MuzzleLocation->SetupAttachment(FP_Gun);
	//FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	TP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("TP_Gun"));
	TP_Gun->SetOwnerNoSee(true);
	TP_Gun->SetupAttachment(GetMesh(), TEXT("hand_rSocket"));

	GetMesh()->SetOwnerNoSee(true);

	//파티클 생성
	TP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("ParticleSysTP"));
	TP_GunShotParticle->bAutoActivate = false;
	TP_GunShotParticle->AttachTo(TP_Gun);
	TP_GunShotParticle->SetOwnerNoSee(true);

	FP_GunShotParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("FP_GunShotParticle"));
	FP_GunShotParticle->bAutoActivate = false;
	FP_GunShotParticle->AttachTo(FP_Gun);
	FP_GunShotParticle->SetOwnerNoSee(true);

	BulletParticle = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("BulletSysTP"));
	BulletParticle->bAutoActivate = false;
	BulletParticle->AttachTo(FirstPersonCameraComponent);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AfpsNSCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind fire event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AfpsNSCharacter::OnFire);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &AfpsNSCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AfpsNSCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AfpsNSCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AfpsNSCharacter::LookUpAtRate);
}

void AfpsNSCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AfpsNSCharacter, CurrentTeam);
}

float AfpsNSCharacter::TakeDamage(float Damage, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(Damage, DamageEvent, EventInstigator, DamageCauser);

	if (GetLocalRole() == ROLE_Authority && DamageCauser != this && NSPlayerState->Health > 0)
	{
		NSPlayerState->Health -= Damage;
		PlayPain();

		if (NSPlayerState->Health <= 0)
		{
			NSPlayerState->Deaths++;

			// 플레이어가 리스폰할 시간 동안 죽는다
			MultiCastRagdoll();
			AfpsNSCharacter* OtherChar = Cast<AfpsNSCharacter>(DamageCauser);

			if (OtherChar)
			{
				OtherChar->NSPlayerState->Score += 1.0f;
			}

			// 3초 뒤 리스폰된다
			FTimerHandle thisTimer;

			GetWorldTimerManager().SetTimer<AfpsNSCharacter>(thisTimer, this, &AfpsNSCharacter::Respawn, 3.0f, false);
		}
	}
	return Damage;
}

void AfpsNSCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	if (GetLocalRole() != ROLE_Authority)
	{
		SetTeam(CurrentTeam);
	}
}

void AfpsNSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	NSPlayerState = Cast<ANSPlayerState>(GetPlayerState());

	if (GetLocalRole() == ROLE_Authority && NSPlayerState != nullptr)
	{
		NSPlayerState->Health = 100.0f;
	}
}

void AfpsNSCharacter::OnFire()
{
	// try and play the sound if specified
	//if (FireSound != nullptr)
	//{
	//	UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	//}

	// try and play a firing animation if specified
	if (FP_FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = FP_Mesh->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FP_FireAnimation, 1.f);
		}
	}

	// 지정됐다면 FP 파티클 이펙트를 재생한다.
	if (FP_GunShotParticle != nullptr)
	{
		FP_GunShotParticle->Activate(true);
	}

	FVector mousePos;
	FVector mouseDir;

	APlayerController* pController = Cast<APlayerController>(GetController());
	FVector2D ScreenPos = GEngine->GameViewport->Viewport->GetSizeXY();

	pController->DeprojectScreenPositionToWorld(ScreenPos.X / 2.0f, ScreenPos.Y / 2.0f, mousePos, mouseDir);
	mouseDir *= 10000000.0f;

	ServerFire(mousePos, mouseDir);
}

void AfpsNSCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void AfpsNSCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}

void AfpsNSCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AfpsNSCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AfpsNSCharacter::Fire(const FVector pos, const FVector dir)
{
	//레이캐스트 수행
	FCollisionObjectQueryParams ObjQuery;
	ObjQuery.AddObjectTypesToQuery(ECC_GameTraceChannel1);

	FCollisionQueryParams ColQuery;
	ColQuery.AddIgnoredActor(this);

	FHitResult HitRes;
	GetWorld()->LineTraceSingleByObjectType(HitRes, pos, dir, ObjQuery, ColQuery);

	DrawDebugLine(GetWorld(), pos, dir, FColor::Red, true, 100, 0, 5.0f);

	if (HitRes.bBlockingHit)
	{
		AfpsNSCharacter* OtherChar = Cast<AfpsNSCharacter>(HitRes.GetActor());
		if (OtherChar != nullptr && OtherChar->GetNSPlayerState()->Team != this->GetNSPlayerState()->Team)
		{
			FDamageEvent thisEvent(UDamageType::StaticClass());
			OtherChar->TakeDamage(10.0f, thisEvent, this->GetController(), this);
			APlayerController* thisPC = Cast<APlayerController>(GetController());
			thisPC->ClientPlayForceFeedback(HitSuccessFeedback, false, NAME_None);
		}
	}
}

bool AfpsNSCharacter::ServerFire_Validate(const FVector pos, const FVector dir)
{
	if (pos != FVector(ForceInit) && dir != FVector(ForceInit))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void AfpsNSCharacter::ServerFire_Implementation(const FVector pos, const FVector dir)
{
	Fire(pos, dir);
	MultiCastShootEffects();
}

void AfpsNSCharacter::MultiCastShootEffects_Implementation()
{
	// 지정됐다면 발사 애니메이션 재생을 시도한다
	if (TP_FireAnimation != nullptr)
	{
		// 팔 메시의 애니메이션 오브젝트를 얻는다
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(TP_FireAnimation, 1.0f);
		}
	}

	// 지정된 경우 사운드 재생을 시도한다.
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	if (TP_GunShotParticle != nullptr)
	{
		TP_GunShotParticle->Activate(true);
	}

	if (BulletParticle != nullptr)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BulletParticle->Template, BulletParticle->GetComponentLocation(), BulletParticle->GetComponentRotation());
	}
}

void AfpsNSCharacter::MultiCastRagdoll_Implementation()
{
	GetMesh()->SetPhysicsBlendWeight(1.0f);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName("Ragdoll");
}

void AfpsNSCharacter::PlayPain_Implementation()
{
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		UGameplayStatics::PlaySoundAtLocation(this, PainSound, GetActorLocation());
	}
}

ANSPlayerState* AfpsNSCharacter::GetNSPlayerState()
{
	if (NSPlayerState)
	{
		return NSPlayerState;
	}
	else
	{
		NSPlayerState = Cast<ANSPlayerState>(GetPlayerState());
		return NSPlayerState;
	}
}

void AfpsNSCharacter::SetNSPlayerState(ANSPlayerState* newPS)
{
	// PS가 유효하고 서버에만 설정됐는지 확인한다.
	if (newPS && GetLocalRole() == ROLE_Authority)
	{
		NSPlayerState = newPS;
		SetPlayerState(newPS);
	}
}

void AfpsNSCharacter::Respawn()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		// 게임 모드로부터 위치 얻기
		NSPlayerState->Health = 100.0f;
		Cast<AfpsNSGameMode>(GetWorld()->GetAuthGameMode())->Respawn(this);
		Destroy(true, true);
	}
}

void AfpsNSCharacter::SetTeam_Implementation(ETeam NewTeam)
{
	FLinearColor outColor;
	if (NewTeam == ETeam::BLUE_TEAM)
	{
		outColor = FLinearColor(0.0f, 0.0f, 0.5f);
	}
	else
	{
		outColor = FLinearColor(0.5f, 0.0f, 0.0f);
	}

	if (DynamicMat == nullptr)
	{
		DynamicMat = UMaterialInstanceDynamic::Create(GetMesh()->GetMaterial(0), this);
		DynamicMat->SetVectorParameterValue(TEXT("BodyColor"), outColor);
		GetMesh()->SetMaterial(0, DynamicMat);
		FP_Mesh->SetMaterial(0, DynamicMat);
	}
}
