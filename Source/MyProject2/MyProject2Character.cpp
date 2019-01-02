// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MyProject2Character.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Life.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Animation/AnimInstance.h"
#include "UObject/UnrealType.h"
#include "EnemyClass.h"
#include "Kismet/GameplayStatics.h"

//////////////////////////////////////////////////////////////////////////
// AMyProject2Character

AMyProject2Character::AMyProject2Character()
{
	PrimaryActorTick.bCanEverTick = true;
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetNotifyRigidBodyCollision(true);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);
	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &AMyProject2Character::OnHit);	

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> CharacterMesh(TEXT("/Game/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	GetMesh()->SetSkeletalMesh(CharacterMesh.Object);
	GetMesh()->SetRelativeLocation(FVector(0,0,-GetCapsuleComponent()->CalcBounds(GetCapsuleComponent()->GetComponentTransform()).BoxExtent.Z));

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
	
	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	_health = CreateDefaultSubobject<ULife>(TEXT("Health"));
	_health->SetHealth(100);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SwordMesh(TEXT("/Game/InfinityBladeWeapons/Weapons/Blade/Swords/Blade_DragonSword/SK_Blade_DragonSword.SK_Blade_DragonSword"));

	Sword = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Sword"));
	Sword->SetSkeletalMesh(SwordMesh.Object);
	Sword->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, "Sword");

	SwordCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("SwordCollision"));
	SwordCollision->AttachToComponent(Sword, FAttachmentTransformRules::SnapToTargetIncludingScale, "SwordCollision");
	SwordCollision->SetBoxExtent(Sword->CalcBounds(Sword->GetComponentTransform()).BoxExtent / 2);
	SwordCollision->SetRelativeLocation(FVector(0, 0, Sword->CalcBounds(Sword->GetComponentTransform()).BoxExtent.Z/2));
	SwordCollision->SetGenerateOverlapEvents(true);
	SwordCollision->SetNotifyRigidBodyCollision(true);
	SwordCollision->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	SwordCollision->SetCollisionObjectType(ECollisionChannel::ECC_PhysicsBody);
	SwordCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	SwordCollision->SetCollisionEnabled(ECollisionEnabled::Type::QueryOnly);
	
	
	text = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthDisplay"));
	text->AttachTo(RootComponent);
	text->SetTextRenderColor(FColor(0,255,0,100));
	text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
	text->WorldSize = 15.0f;
	text->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	text->bAbsoluteRotation = true;
	text->SetRelativeLocation(FVector(0,0, GetMesh()->CalcBounds(GetMesh()->GetComponentTransform()).BoxExtent.Z));
	

	GetCharacterMovement()->MaxWalkSpeed = 150.0f;

	DamageCooldown = 0.0f;

	
	strongAttack = false;
	weakAttack = false;
	defend = false;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyProject2Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction("StrongAttack", IE_Pressed, this, &AMyProject2Character::StrongAttack);
	PlayerInputComponent->BindAction("WeakAttack", IE_Pressed, this, &AMyProject2Character::WeakAttack);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMyProject2Character::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyProject2Character::MoveRight);
	PlayerInputComponent->BindAxis("Sprint", this, &AMyProject2Character::Run);
	PlayerInputComponent->BindAxis("Defend", this, &AMyProject2Character::Defend);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AMyProject2Character::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AMyProject2Character::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyProject2Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyProject2Character::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AMyProject2Character::OnResetVR);
}

void AMyProject2Character::BeginPlay()
{
	Super::BeginPlay();
	BPattack = FindField<UBoolProperty>(GetMesh()->GetAnimInstance()->GetClass(), "StopAttack");
	BPcounter = FindField<UBoolProperty>(GetMesh()->GetAnimInstance()->GetClass(), "Counter");
}

void AMyProject2Character::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (DamageCooldown > 0.0f)
		DamageCooldown -= DeltaTime;

	if (BPattack != NULL)
	{
		if (BPattack->GetPropertyValue_InContainer(GetMesh()->GetAnimInstance()) == true)
		{
			SwordCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
			StopAttack();
		}
		else
			SwordCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
	}
	
	text->SetWorldRotation(FQuat(FRotator(0,-180 + Controller->GetControlRotation().Yaw,0)));
}

void AMyProject2Character::OnResetVR()
{
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AMyProject2Character::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AMyProject2Character::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AMyProject2Character::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMyProject2Character::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AMyProject2Character::MoveForward(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMyProject2Character::MoveRight(float Value)
{
	if ((Controller != NULL) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AMyProject2Character::StrongAttack()
{
	strongAttack = true;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyClass::StaticClass(), foundEnemies);

	for (AActor* enemies : foundEnemies)
	{
		enemy = Cast<AEnemyClass>(enemies);
		enemy->damage = 10.0f;
	}
}

void AMyProject2Character::WeakAttack()
{
	weakAttack = true;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyClass::StaticClass(), foundEnemies);

	for (AActor* enemies : foundEnemies)
	{
		enemy = Cast<AEnemyClass>(enemies);
		enemy->damage = 5.0f;
	}
}

void AMyProject2Character::StopAttack()
{
	strongAttack = false;
	weakAttack = false;
}

void AMyProject2Character::Defend(float Value)
{
	if (Value != 0)
		defend = true;
	else
		defend = false;
}

void AMyProject2Character::Run(float Value)
{
	if (Value != 0)	
		GetCharacterMovement()->MaxWalkSpeed = 600.0f;
	else
		GetCharacterMovement()->MaxWalkSpeed = 200.0f;
}

void AMyProject2Character::OnHit(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult &SweepResult)
{
	if ((DamageCooldown <= 0.0f) && (OtherComp != SwordCollision))
	{
		enemy = Cast<AEnemyClass>(OtherActor);
		Damage();
	}
}

void AMyProject2Character::Damage()
{
	if (BPcounter->GetPropertyValue_InContainer(GetMesh()->GetAnimInstance()) == false)
	{
		if (defend == false)
			_health->UpdateHealth(enemy->attackDamage);
		else
			_health->UpdateHealth(enemy->attackDamage/2.0f);
	}
	else
		StrongAttack();
	DamageCooldown = 3.0f;
	text->SetText(FString("Health: ") + FString::SanitizeFloat(_health->Health));
}
