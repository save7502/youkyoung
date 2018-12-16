// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Engine/World.h"
#include "Stats/Stats.h"
#include "Containers/Set.h"
#include "Engine/EngineTypes.h"
#include "IAnimationBudgetAllocator.h"
#include "UObject/GCObject.h"

DECLARE_STATS_GROUP(TEXT("Animation Budget Allocator"), STATGROUP_AnimationBudgetAllocator, STATCAT_Advanced);

#define USE_SKEL_BATCHING 1
#define WITH_TICK_DEBUG 0

// Enable this define to output more dense CSV stats about the budgeter
#define WITH_EXTRA_BUDGET_CSV_STATS		WITH_TICK_DEBUG

#if WITH_EXTRA_BUDGET_CSV_STATS
#define BUDGET_CSV_STAT(Category,StatName,Value,Op)	CSV_CUSTOM_STAT(Category,StatName,Value,Op)
#else
#define BUDGET_CSV_STAT(Category,StatName,Value,Op)	
#endif

class FAnimationBudgetAllocator;
class USkeletalMeshComponentBudgeted;

/** Data for a single component */
struct FComponentData
{
	FComponentData()
		: Component(nullptr)
		, RootPrerequisite(nullptr)
		, Significance(0.0f)
		, AccumulatedDeltaTime(0.0f)
		, GameThreadLastTickTimeMs(0.0f)
		, GameThreadLastCompletionTimeMs(0.0f)
		, FrameOffset(0)
		, DesiredTickRate(0)
		, TickRate(0)		
		, SkippedTicks(0)
		, StateChangeThrottle(0)
		, bTickEnabled(false)
		, bAlwaysTick(false)
		, bTickEvenIfNotRendered(false)
		, bInterpolate(false)
		, bReducedWork(false)
		, bAllowReducedWork(false)
		, bAutoCalculateSignificance(false)
		, bOnScreen(false)
		, bNeverThrottle(true)
	{}

	FComponentData(USkeletalMeshComponentBudgeted* InComponent);

	bool operator==(const FComponentData& InOther) const
	{
		return Component == InOther.Component;
	}

public:
	/** The component that we are tracking */
	USkeletalMeshComponentBudgeted* Component;

	/** The root skeletal mesh component of this component's prerequisite graph, used for synchronizing ticks */
	USkeletalMeshComponentBudgeted* RootPrerequisite;

	/** Significance of this component */
	float Significance;

	/** Delta time accumulated between ticks we miss */
	float AccumulatedDeltaTime;

	/** Tracks the time in MS it took to tick this component on the game thread */
	float GameThreadLastTickTimeMs;

	/** Tracks the time in MS it took to complete this component on the game thread */
	float GameThreadLastCompletionTimeMs;

	/** Frame offset used to distribute ticks */
	uint32 FrameOffset;

	/** The tick rate we calculated for this component */
	uint8 DesiredTickRate;

	/** The tick rate we are using for this component */
	uint8 TickRate;

	/** The current number of skipped ticks, used for determining interpolation alpha */
	uint8 SkippedTicks;

	/** Counter used to prevent state changes from happening too often */
	uint8 StateChangeThrottle;

	/** Whether we ever tick */
	uint8 bTickEnabled : 1;

	/** Whether we should never skip the tick of this component, e.g. for player pawns */
	uint8 bAlwaysTick : 1;

	/** Whether we should always try to tick this component offscreen, e.g. for meshes with important audio notifies */
	uint8 bTickEvenIfNotRendered : 1;

	/** Whether we should interpolate */
	uint8 bInterpolate : 1;

	/** Whether this component is running 'reduced work' */
	uint8 bReducedWork : 1;

	/** Whether this component allows 'reduced work' */
	uint8 bAllowReducedWork : 1;

	/** Whether this component auto-calculates its significance (as opposed to it being pushed via SetComponentSignificance() */
	uint8 bAutoCalculateSignificance : 1;

	/** Whether this component is on screen. This is updated each tick. */
	uint8 bOnScreen : 1;

	/** Whether we are allowing interpolation on this component (i.e. we dont just reduce tick rate). This is intended to allow higher-quality animation. */
	uint8 bNeverThrottle : 1;
};

class ANIMATIONBUDGETALLOCATOR_API FAnimationBudgetAllocator : public IAnimationBudgetAllocator, public FGCObject
{
public:
	FAnimationBudgetAllocator(UWorld* InWorld);
	~FAnimationBudgetAllocator();

	// IAnimationBudgetAllocator interface
	virtual void RegisterComponent(USkeletalMeshComponentBudgeted* InComponent) override;
	virtual void UnregisterComponent(USkeletalMeshComponentBudgeted* InComponent) override;
	virtual void UpdateComponentTickPrerequsites(USkeletalMeshComponentBudgeted* InComponent) override;
	virtual void SetComponentSignificance(USkeletalMeshComponentBudgeted* Component, float Significance, bool bNeverSkip = false, bool bTickEvenIfNotRendered = false, bool bAllowReducedWork = true, bool bForceInterpolate = false) override;
	virtual void SetComponentTickEnabled(USkeletalMeshComponentBudgeted* Component, bool bShouldTick) override;
	virtual void SetGameThreadLastTickTimeMs(int32 InManagerHandle, float InGameThreadLastTickTimeMs) override;
	virtual void SetGameThreadLastCompletionTimeMs(int32 InManagerHandle, float InGameThreadLastCompletionTimeMs) override;
	virtual void SetIsRunningReducedWork(USkeletalMeshComponentBudgeted* Component, bool bInReducedWork) override;
	virtual void Update(float DeltaSeconds) override;
	virtual void SetEnabled(bool bInEnabled) override;
	virtual bool GetEnabled() const override;

	// FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

protected:
	/** We tick before all actors in the world using this delegate */
	void OnWorldPreActorTick(UWorld* InWorld, ELevelTick InLevelTick, float InDeltaSeconds);

	/** Clean up dead components post GC */
	void HandlePostGarbageCollect();
 
	/** First pass of the Update(). Queues component indices that want to tick into AllSortedComponentData. */
	void QueueSortedComponentIndices(float InDeltaSeconds);

	/**
	 * Second pass of the Update(). Looks at average time taken per unit of work (i.e. a component tick & completion task) and
	 * determines appropriate tick rates to suit the requested time budget.
	 */
	int32 CalculateWorkDistributionAndQueue(float InDeltaSeconds, float& OutAverageTickRate);

	/** Helper function for keeping handle indices in sync */
	void RemoveHelper(int32 Index);

	/** Helper function to enable/disable ticks */
	void TickEnableHelper(USkeletalMeshComponent* InComponent, bool bInEnable);

protected:
	// World we are linked to
	UWorld* World;

	// All component data
	TArray<FComponentData> AllComponentData;

	/** All currently tickable component indices sorted by significance, updated each tick */
	TArray<int32> AllSortedComponentData;

#if WITH_TICK_DEBUG
	TArray<FComponentData*> AllSortedComponentDataDebug;
#endif

	/** All components that have reduced work that might want to tick (and hence might not want to do reduced work) */
	TArray<int32> ReducedWorkComponentData;

	/** All components that have reduced work that must now tick */
	TArray<int32> DisallowedReducedWorkComponentData;

	/** All non-rendered components we might tick */
	TArray<int32> NonRenderedComponentData;

	/** Average time for a work unit in milliseconds (smoothed). Updated each tick. */
	float AverageWorkUnitTimeMs;

	/** The number of components that we need to tick very frame. Updated each tick. */
	int32 NumComponentsToNotSkip;

	/** The number of components that we should not throttle (i.e. interpolate). Updated each tick. */
	int32 NumComponentsToNotThrottle;

	/** The total estimated tick time for queued ticks this frame. Updated each tick. */
	float TotalEstimatedTickTimeMs;

	/** The number of work units queued for tick this frame, used to calculate target AverageWorkUnitTimeMs. Updated each tick */
	float NumWorkUnitsForAverage;

	/** Handle used to track garbage collection */
	FDelegateHandle PostGarbageCollectHandle;

	/** Handle used for ticking */
	FDelegateHandle OnWorldPreActorTickHandle;

	/** Offset used to distribute component ticks */
	uint32 CurrentFrameOffset;

	/** Local enabled flag that allows us to disable even if the CVar is enabled */
	bool bEnabled;

	/** Cached enabled flag that is copied from the CVar each tick */
	static bool bCachedEnabled;
};
