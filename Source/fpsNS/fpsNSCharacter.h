// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "fpsNSGameMode.h"
#include "NSPlayerState.h"
#include "GameFramework/Character.h"
#include "fpsNSCharacter.generated.h"

class UInputComponent;
class USkeletalMeshComponent;
class USceneComponent;
class UCameraComponent;
class UMotionControllerComponent;
class UAnimMontage;
class USoundBase;

UCLASS(config=Game)
class AfpsNSCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	USkeletalMeshComponent* FP_Mesh;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* FP_Gun;

	/** Gun mesh: 3st person view (seen only by others) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USkeletalMeshComponent* TP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** Motion controller (right hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* R_MotionController;

	/** Motion controller (left hand) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	UMotionControllerComponent* L_MotionController;

public:
	AfpsNSCharacter();

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	virtual void BeginPlay();
	virtual void PossessedBy(AController* NewController) override;

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* PainSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FP_FireAnimation;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* TP_FireAnimation;

	/** 총 발사 효과를 위한 1인칭 파티클 시스템 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UParticleSystemComponent* FP_GunShotParticle;

	/** 총 발사 효과를 위한 3인칭 파티클 시스템 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UParticleSystemComponent* TP_GunShotParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UParticleSystemComponent* BulletParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UForceFeedbackEffect* HitSuccessFeedback;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint8 bUsingMotionControllers : 1;
	
public:
	UPROPERTY(Replicated, BlueprintReadWrite, Category = Team)
	ETeam CurrentTeam;

protected:
	class UMaterialInstanceDynamic* DynamicMat;
	class ANSPlayerState* NSPlayerState;
	
	/** Fires a projectile. */
	void OnFire();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	// 레이트레이스를 서버에서 수행하기 위해 호출된다
	void Fire(const FVector pos, const FVector dir);

private:
	// 서버에서 fire 액션 수행
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerFire(const FVector pos, const FVector dir);
	bool ServerFire_Validate(const FVector pos, const FVector dir);
	void ServerFire_Implementation(const FVector pos, const FVector dir);

	// 모든 클라이언트가 발사 효과를 실행하는 멀티캐스트
	UFUNCTION(NetMultiCast, unreliable)
	void MultiCastShootEffects();
	void MultiCastShootEffects_Implementation();

	// 사망시 모든 클라이언트에게 사망을 전달하기 위해 호출된다
	UFUNCTION(NetMultiCast, unreliable)
	void MultiCastRagdoll();
	void MultiCastRagdoll_Implementation();

	// 히트시 소유 클라이언트에게 고통을 준다
	UFUNCTION(Client, Reliable)
	void PlayPain();
	void PlayPain_Implementation();

public:
	/** Returns Mesh1P subobject **/
	//USkeletalMeshComponent* GetMesh() const { return FP_Mesh; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	class ANSPlayerState* GetNSPlayerState();
	void SetNSPlayerState(class ANSPlayerState* newPS);
	void Respawn();

	//팀 색상 설정
	UFUNCTION(NetMultiCast, Reliable)
	void SetTeam(ETeam NewTeam);
	void SetTeam_Implementation(ETeam NewTeam);

};

