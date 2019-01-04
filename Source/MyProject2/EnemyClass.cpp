// Fill out your copyright notice in the Description page of Project Settings.

#include "EnemyClass.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "UObject/ConstructorHelpers.h"
#include "Life.h"
#include "Components/TextRenderComponent.h"
#include "UObject/UnrealType.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject2Character.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Perception/PawnSensingComponent.h"

// Sets default values
AEnemyClass::AEnemyClass()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	GetCapsuleComponent()->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	GetCapsuleComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Destructible, ECollisionResponse::ECR_Ignore);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> EnemyMesh(TEXT("/Game/InfinityBladeAdversaries/Enemy/Enemy_Golem/SK_Fire_Golem.SK_Fire_Golem"));
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> WeaponMesh(TEXT("/Game/InfinityBladeAdversaries/Enemy/Enemy_Golem/SK_Enemy_Golem01_Weapon.SK_Enemy_Golem01_Weapon"));

	GetCharacterMovement()->bAutoActivate = true;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	

	AImovement = CreateDefaultSubobject<UPawnSensingComponent>(TEXT("Pawn_Sensor"));
	AImovement->SensingInterval = 0.0000001f;
	AImovement->bOnlySensePlayers = true;
	AImovement->SetPeripheralVisionAngle(50.0f);
	AImovement->SightRadius = 800.f;
	AImovement->HearingThreshold = 1000.f;
	AImovement->LOSHearingThreshold = 900.f;
	AImovement->OnSeePawn.AddDynamic(this, &AEnemyClass::SeePlayer);
	AImovement->OnHearNoise.AddDynamic(this, &AEnemyClass::HearPlayer);

	box = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));

	box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	box->AttachTo(GetCapsuleComponent());
	box->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
	box->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	box->SetGenerateOverlapEvents(true);
	box->SetNotifyRigidBodyCollision(true);
	box->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	GetMesh()->SetSkeletalMesh(EnemyMesh.Object);
	GetMesh()->AttachTo(box);

	weaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Weapon"));
	weaponMesh->SetSkeletalMesh(WeaponMesh.Object);

	attackCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Attack_Collision"));
	attackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	attackCollision->SetCollisionObjectType(ECollisionChannel::ECC_Destructible);
	attackCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	attackCollision->SetGenerateOverlapEvents(true);

	_health = CreateDefaultSubobject<ULife>(TEXT("Health"));
	_health->SetHealth(100);

	text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthDisplay"));
	text->AttachTo(box);
	text->SetTextRenderColor(FColor(255, 0, 0, 100));
	text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
	text->WorldSize = 20.0f;

	DamageCooldown = 0.0f;

	damage = 0.0f;

	box->OnComponentBeginOverlap.AddDynamic(this, &AEnemyClass::Damaged);
	box->OnComponentHit.AddDynamic(this, &AEnemyClass::AttackPlayer);

	attack = false;
	attackDamage = 5.0f;
	MaximumSpeed = 200.0f;
	enemyVision = AImovement->GetPeripheralVisionAngle();
}

// Called when the game starts or when spawned
void AEnemyClass::BeginPlay()
{
	Super::BeginPlay();
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyProject2Character::StaticClass(), foundCharacter);

	for (AActor* player : foundCharacter)
	{
		character = Cast<AMyProject2Character>(player);
	}
	GetCapsuleComponent()->SetCapsuleSize(1.0f, GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent.Z / 2.0f);
	box->SetBoxExtent(GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent / 2);
	GetMesh()->SetRelativeLocation(FVector(0, 0, -GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent.Z / 2.0f));
	text->SetRelativeLocationAndRotation(FVector(0, 0, GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent.Z), FQuat(FRotator(0, 90.0f, 0)));
	text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	if (GetMesh()->SkeletalMesh->GetName() == "SK_Fire_Golem")
	{
		box->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		weaponMesh->AttachTo(GetMesh(), "weapon");
		weaponMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
		attackCollision->AttachTo(weaponMesh);
		attackCollision->SetBoxExtent(FVector(10.0f, 10.0f, 45.0f));
		attackCollision->SetRelativeLocation(FVector(0, 0, 40.0f));
		_health->SetHealth(50);
		text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
		BPstopAttack = FindField<UBoolProperty>(GetMesh()->GetAnimInstance()->GetClass(), "StopAttack");
		BPdisableMesh = FindField<UBoolProperty>(GetMesh()->GetAnimInstance()->GetClass(), "disableMesh");
		BPotherAnimation = FindField<UBoolProperty>(GetMesh()->GetAnimInstance()->GetClass(), "rolling");
		MaximumSpeed = 300.0f;
		enemyVision = AImovement->GetPeripheralVisionAngle();
	}
	else if (GetMesh()->SkeletalMesh->GetName() == "Enemy_Bear")
	{
		box->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		weaponMesh->DestroyComponent();
		attackCollision->AttachTo(GetMesh(), "up_arm_rt");
		attackCollision->SetBoxExtent(FVector(65.0f, 40.0f, 28.0f));
		attackCollision->SetRelativeLocationAndRotation(FVector(-40.0f, 20.0f, 10.0f), FRotator(-20.0f, -20.0f, 0.0f));
		_health->SetHealth(70);
		text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
		attackDamage = 7.0f;
		BPstopAttack = FindField<UBoolProperty>(GetMesh()->GetAnimInstance()->GetClass(), "StopAttack");
		BPdisableMesh = FindField<UBoolProperty>(GetMesh()->GetAnimInstance()->GetClass(), "disableMesh");
		MaximumSpeed = 200.0f;
		enemyVision = AImovement->GetPeripheralVisionAngle();
	}
}

// Called every frame
void AEnemyClass::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DamageCooldown > 0.0f)
		DamageCooldown -= DeltaTime;

	if (_health->GetHealth() <= 0.0f)
		Destroy();

	if (attack == true)
	{
		if (BPstopAttack->GetPropertyValue_InContainer(GetMesh()->GetAnimInstance()) == true)
		{
			attack = false;
			attackCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		}
		GetCharacterMovement()->MaxWalkSpeed = 0.0001f;
	}
	else if (attackCollision->GetCollisionEnabled() == ECollisionEnabled::QueryOnly)
	{
		if (BPdisableMesh->GetPropertyValue_InContainer(GetMesh()->GetAnimInstance()) == true)
		{
			attackCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		GetCharacterMovement()->MaxWalkSpeed = 0.0001f;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = MaximumSpeed;
	}
	if (BPotherAnimation != NULL)
	{
		if (BPotherAnimation->GetPropertyValue_InContainer(GetMesh()->GetAnimInstance()) == false)
		{
			GetCharacterMovement()->MaxWalkSpeed = 0.01f;
		}

	}
}

void AEnemyClass::Damaged(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	if ((DamageCooldown <= 0.0f) && (OtherComp != attackCollision))
	{
		_health->UpdateHealth(damage);
		DamageCooldown = 2.0f;
		text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
	}
}

void AEnemyClass::AttackPlayer(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == character)
	{
		attack = true;
	}
}
void  AEnemyClass::SeePlayer(APawn* Character)
{
	if ((character = Cast<AMyProject2Character>(Character)) != NULL)
	{
		FVector DistanceBetweenActors = (character->GetActorLocation() - GetActorLocation());
		FRotator Rotation = FRotationMatrix::MakeFromX(DistanceBetweenActors).Rotator();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, 1.0f, true);
	}	
}
void AEnemyClass::HearPlayer(APawn* Character, const FVector& location, float volume)
{
	if ((character = Cast<AMyProject2Character>(Character)) != NULL)
	{
		FVector Angle = (character->GetActorLocation() - GetActorLocation());
		FRotator Rotation = FRotationMatrix::MakeFromX(Angle).Rotator();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, 1.0f, true);
	}
}

void AEnemyClass::Destroy()
{
	GetCapsuleComponent()->DestroyComponent();
	attackCollision->DestroyComponent();
	text->DestroyComponent();
	GetMesh()->DestroyComponent();
	box->DestroyComponent();
	if (weaponMesh != NULL)
		weaponMesh->DestroyComponent();
}

