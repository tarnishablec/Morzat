﻿// Copyright 2019-Present tarnishablec. All Rights Reserved.


#include "TreeckoStateComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "StateTreeExecutionContext.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Treecko/Schema/TreeckoStateSchema.h"


// Sets default values for this component's properties
UTreeckoStateComponent::UTreeckoStateComponent()
{
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;
    bWantsInitializeComponent = true;

    // ...
}

TSubclassOf<UStateTreeSchema> UTreeckoStateComponent::GetSchema() const
{
    return UTreeckoStateSchema::StaticClass();
}

void UTreeckoStateComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UTreeckoStateComponent::InitializeComponent()
{
    Super::InitializeComponent();
}

void UTreeckoStateComponent::StartLogic()
{
    UpdateActorContext();
    Super::StartLogic();
}

void UTreeckoStateComponent::BeginDestroy()
{
    StopLogic("");
    Super::BeginDestroy();
}

void UTreeckoStateComponent::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    constexpr FDoRepLifetimeParams Params{
        .Condition = COND_OwnerOnly,
        .RepNotifyCondition = REPNOTIFY_OnChanged,
        .bIsPushBased = true
    };
    DOREPLIFETIME_WITH_PARAMS(ThisClass, ActorContext, Params);
    DOREPLIFETIME_WITH_PARAMS(ThisClass, AbilitySystemComponent, Params);
}

bool UTreeckoStateComponent::SetContextRequirements(FStateTreeExecutionContext& Context, const bool bLogErrors)
{
    // auto Result = Super::SetContextRequirements(Context, bLogErrors);
    bool Result = Context.IsValid();

    Context.SetLinkedStateTreeOverrides(&LinkedStateTreeOverrides);

    Context.SetContextDataByName(Treecko::FStateTreeContextDataNames::ContextOwner, ActorContext.Owner.Get());
    Context.SetContextDataByName(Treecko::FStateTreeContextDataNames::ContextAvatar, ActorContext.Avatar.Get());
    Context.SetContextDataByName(Treecko::FStateTreeContextDataNames::ContextStateTreeComponent, this);
    Context.SetContextDataByName(Treecko::FStateTreeContextDataNames::ContextMeshComponent,
                                 ActorContext.MeshComponent.Get());
    Context.SetContextDataByName(Treecko::FStateTreeContextDataNames::ContextAbilitySystemComponent,
                                 ActorContext.AbilitySystemComponent.Get());

    const auto* Schema = CastChecked<UTreeckoStateSchema>(Context.GetStateTree()->GetSchema());

    Result &= ActorContext.Owner && ActorContext.Owner->IsA(Schema->OwnerType);
    Result &= ActorContext.Avatar && ActorContext.Avatar->IsA(Schema->AvatarType);
    Result &= this->IsA(Schema->StateTreeComponentType);
    Result &= !!ActorContext.MeshComponent;
    Result &= !!ActorContext.AbilitySystemComponent;

    if (!Result)
    {
        UE_LOG(LogTemp, Warning, TEXT("Treecko Context Requirement not Satisfied"))
    }

    return Result;
}

void UTreeckoStateComponent::SetAbilitySystemComponent(
    UAbilitySystemComponent* InAbilitySystemComponent)
{
    COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, AbilitySystemComponent, InAbilitySystemComponent, this);
}

void UTreeckoStateComponent::UpdateActorContext_Implementation()
{
    FTreeckoStateTreeActorContext NewContext{};
    NewContext.Owner = GetOwner();
    NewContext.AbilitySystemComponent = SearchAbilitySystemComponent();

    if (NewContext.AbilitySystemComponent)
    {
        NewContext.Avatar = NewContext.AbilitySystemComponent->GetAvatarActor();
        NewContext.MeshComponent = NewContext.AbilitySystemComponent->AbilityActorInfo->
                                              SkeletalMeshComponent.Get();
    }
    COMPARE_ASSIGN_AND_MARK_PROPERTY_DIRTY(ThisClass, ActorContext, NewContext, this);
    OnActorContextUpdated.Broadcast();
}

UAbilitySystemComponent* UTreeckoStateComponent::SearchAbilitySystemComponent_Implementation()
{
    const auto Owner = GetOwner();

    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent;
    }

    UAbilitySystemComponent* Result = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);

    if (Result)
    {
        return Result;
    }

    if (const auto PawnOwner = Cast<APawn>(Owner))
    {
        if (const auto Controller = PawnOwner->GetController())
        {
            if (const auto PC = Cast<APlayerController>(Controller))
            {
                if (const auto PS = PC->PlayerState)
                {
                    Result = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
                    if (Result)
                    {
                        return Result;
                    }
                }
            }
        }
    }

    if (const auto ControllerOwner = Cast<AController>(Owner))
    {
        if (const auto Pawn = ControllerOwner->GetPawn())
        {
            Result = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn);
            if (Result)
            {
                return Result;
            }
        }

        if (const auto PCOwner = Cast<APlayerController>(ControllerOwner))
        {
            if (const auto PS = PCOwner->PlayerState)
            {
                Result = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
                if (Result)
                {
                    return Result;
                }
            }
        }
    }

    return Result;
}
