// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

//
// Base class of a network driver attached to an active or pending level.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Object.h"
#include "Misc/NetworkGuid.h"
#include "UObject/CoreNet.h"
#include "GameFramework/WorldSettings.h"
#include "PacketHandler.h"
#include "Channel.h"
#include "DDoSDetection.h"
#include "IPAddress.h"

#include "NetDriver.generated.h"

class Error;
class FNetGUIDCache;
class FNetworkNotify;
class FNetworkObjectList;
class FObjectReplicator;
class FRepChangedPropertyTracker;
class FRepLayout;
class FReplicationChangelistMgr;
class FVoicePacket;
class StatelessConnectHandlerComponent;
class UNetConnection;
class UReplicationDriver;
struct FNetworkObjectInfo;
class UChannel;
class IAnalyticsProvider;
class FNetAnalyticsAggregator;

using FConnectionMap = TMap<TSharedRef<FInternetAddr>, UNetConnection*, FDefaultSetAllocator, FInternetAddrKeyMapFuncs<UNetConnection*>>;

extern ENGINE_API TAutoConsoleVariable<int32> CVarNetAllowEncryption;
extern ENGINE_API int32 GNumSaturatedConnections;
extern ENGINE_API int32 GNumSharedSerializationHit;
extern ENGINE_API int32 GNumSharedSerializationMiss;
extern ENGINE_API int32 GNumReplicateActorCalls;
extern ENGINE_API bool GReplicateActorTimingEnabled;
extern ENGINE_API bool GReceiveRPCTimingEnabled;
extern ENGINE_API double GReplicateActorTimeSeconds;
extern ENGINE_API uint32 GNetOutBytes;
extern ENGINE_API double GReplicationGatherPrioritizeTimeSeconds;
extern ENGINE_API double GServerReplicateActorTimeSeconds;
extern ENGINE_API int32 GNumClientConnections;
extern ENGINE_API int32 GNumClientUpdateLevelVisibility;

// Delegates

#if !UE_BUILD_SHIPPING
/**
 * Delegate for hooking ProcessRemoteFunction (used by NetcodeUnitTest)
 *
 * @param Actor				The actor the RPC will be called in
 * @param Function			The RPC to call
 * @param Parameters		The parameters data blob
 * @param OutParms			Out parameter information (irrelevant for RPC's)
 * @param Stack				The script stack
 * @param SubObject			The sub-object the RPC is being called in (if applicable)
 * @param bBlockSendRPC		Whether or not to block sending of the RPC (defaults to false)
 */
DECLARE_DELEGATE_SevenParams(FOnSendRPC, AActor* /*Actor*/, UFunction* /*Function*/, void* /*Parameters*/,
									FOutParmRec* /*OutParms*/, FFrame* /*Stack*/, UObject* /*SubObject*/, bool& /*bBlockSendRPC*/);
#endif

//
// Whether to support net lag and packet loss testing.
//
#define DO_ENABLE_NET_TEST !(UE_BUILD_SHIPPING || UE_BUILD_TEST)


/** Holds the packet simulation settings in one place */
USTRUCT()
struct ENGINE_API FPacketSimulationSettings
{
	GENERATED_USTRUCT_BODY()

	/**
	 * When set, will cause calls to FlushNet to drop packets.
	 * Value is treated as % of packets dropped (i.e. 0 = None, 100 = All).
	 * No general pattern / ordering is guaranteed.
	 * Clamped between 0 and 100.
	 *
	 * Works with all other settings.
	 */
	UPROPERTY(EditAnywhere, Category="Simulation Settings")
	int32	PktLoss;

	/**
	* Sets the maximum size of packets in bytes that will be dropped
	* according to the PktLoss setting. Default is INT_MAX.
	*
	* Works with all other settings.
	*/
	UPROPERTY(EditAnywhere, Category = "Simulation Settings")
	int32	PktLossMaxSize;

	/**
	* Sets the minimum size of packets in bytes that will be dropped
	* according to the PktLoss setting. Default is 0.
	*
	* Works with all other settings.
	*/
	UPROPERTY(EditAnywhere, Category = "Simulation Settings")
	int32	PktLossMinSize;

	/**
	 * When set, will cause calls to FlushNet to change ordering of packets at random.
	 * Value is treated as a bool (i.e. 0 = False, anything else = True).
	 * This works by randomly selecting packets to be delayed until a subsequent call to FlushNet.
	 *
	 * Takes precedence over PktDup and PktLag.
	 */
	UPROPERTY(EditAnywhere, Category="Simulation Settings")
	int32	PktOrder;

	/**
	 * When set, will cause calls to FlushNet to duplicate packets.
	 * Value is treated as % of packets duplicated (i.e. 0 = None, 100 = All).
	 * No general pattern / ordering is guaranteed.
	 * Clamped between 0 and 100.
	 *
	 * Cannot be used with PktOrder or PktLag.
	 */
	UPROPERTY(EditAnywhere, Category="Simulation Settings")
	int32	PktDup;
	
	/**
	 * When set, will cause calls to FlushNet to delay packets.
	 * Value is treated as millisecond lag.
	 *
	 * Cannot be used with PktOrder.
	 */
	UPROPERTY(EditAnywhere, Category="Simulation Settings")
	int32	PktLag;
	
	/**
	 * When set, will cause PktLag to use variable lag instead of constant.
	 * Value is treated as millisecond lag range (e.g. -GivenVariance <= 0 <= GivenVariance).
	 * Clamped between 0 and 100.
	 *
	 * Can only be used when PktLag is enabled.
	 */
	UPROPERTY(EditAnywhere, Category="Simulation Settings")
	int32	PktLagVariance;

	/** Ctor. Zeroes the settings */
	FPacketSimulationSettings() : 
		PktLoss(0),
		PktLossMaxSize(INT_MAX/8),
		PktLossMinSize(0),
		PktOrder(0),
		PktDup(0),
		PktLag(0),
		PktLagVariance(0) 
	{
	}

	/** reads in settings from the .ini file 
	 * @note: overwrites all previous settings
	 */
	void LoadConfig(const TCHAR* OptionalQualifier = nullptr);

	/**
	 * Registers commands for auto-completion, etc.
	 */
	void RegisterCommands();

	/**
	 * Unregisters commands for auto-completion, etc.
	 */
	void UnregisterCommands();

	/**
	 * Reads the settings from a string: command line or an exec
	 *
	 * @param Stream the string to read the settings from
	 * @Param OptionalQualifier: optional string to prepend to Pkt* settings. E.g, "GameNetDriverPktLoss=50"
	 */
	bool ParseSettings(const TCHAR* Stream, const TCHAR* OptionalQualifier=nullptr);

	bool ParseHelper(const TCHAR* Cmd, const TCHAR* Name, int32& Value, const TCHAR* OptionalQualifier);

	bool ConfigHelperInt(const TCHAR* Name, int32& Value, const TCHAR* OptionalQualifier);
	bool ConfigHelperBool(const TCHAR* Name, bool& Value, const TCHAR* OptionalQualifier);
};

//
// Priority sortable list.
//
struct FActorPriority
{
	int32						Priority;	// Update priority, higher = more important.
	
	FNetworkObjectInfo*			ActorInfo;	// Actor info.
	class UActorChannel*		Channel;	// Actor channel.

	struct FActorDestructionInfo *	DestructionInfo;	// Destroy an actor

	FActorPriority() : 
		Priority(0), ActorInfo(NULL), Channel(NULL), DestructionInfo(NULL)
	{}

	FActorPriority(class UNetConnection* InConnection, class UActorChannel* InChannel, FNetworkObjectInfo* InActorInfo, const TArray<struct FNetViewer>& Viewers, bool bLowBandwidth);
	FActorPriority(class UNetConnection* InConnection, struct FActorDestructionInfo * DestructInfo, const TArray<struct FNetViewer>& Viewers );
};

struct FCompareFActorPriority
{
	FORCEINLINE bool operator()( const FActorPriority& A, const FActorPriority& B ) const
	{
		return B.Priority < A.Priority;
	}
};

struct FActorDestructionInfo
{
public:
	FActorDestructionInfo() : Reason(EChannelCloseReason::Destroyed) {}

	TWeakObjectPtr<ULevel>		Level;
	TWeakObjectPtr<UObject>		ObjOuter;
	FVector			DestroyedPosition;
	FNetworkGUID	NetGUID;
	FString			PathName;
	FName			StreamingLevelName;
	EChannelCloseReason Reason;
};

/** Used to specify properties of a channel type */
USTRUCT()
struct ENGINE_API FChannelDefinition
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName ChannelName;			// Channel type identifier

	UPROPERTY()
	FName ClassName;			// UClass name used to create the UChannel

	UPROPERTY()
	UClass* ChannelClass;		// UClass used to create the UChannel

	UPROPERTY()
	int32 StaticChannelIndex;	// Channel always uses this index, INDEX_NONE if dynamically chosen

	UPROPERTY()
	bool bTickOnCreate;			// Whether to immediately begin ticking the channel after creation

	UPROPERTY()
	bool bServerOpen;			// Channel opened by the server

	UPROPERTY()
	bool bClientOpen;			// Channel opened by the client

	UPROPERTY()
	bool bInitialServer;		// Channel created on server when connection is established

	UPROPERTY()
	bool bInitialClient;		// Channel created on client before connecting

	FChannelDefinition() : 
		ChannelName(NAME_None),
		ClassName(NAME_None),
		ChannelClass(nullptr),
		StaticChannelIndex(INDEX_NONE),
		bTickOnCreate(false),
		bServerOpen(false),
		bClientOpen(false),
		bInitialServer(false),
		bInitialClient(false)
	{
	}
};

/**
 * Information about disconnected client NetConnection's
 */
struct FDisconnectedClient
{
	/** The address of the client */
	TSharedRef<FInternetAddr>	Address;

	/** The time at which the client disconnected  */
	double						DisconnectTime;

	FDisconnectedClient(TSharedRef<FInternetAddr>& InAddress, double InDisconnectTime)
		: Address(InAddress)
		, DisconnectTime(InDisconnectTime)
	{
	}
};


UCLASS(Abstract, customConstructor, transient, MinimalAPI, config=Engine)
class UNetDriver : public UObject, public FExec
{
	GENERATED_UCLASS_BODY()

protected:

	ENGINE_API void InternalProcessRemoteFunction(class AActor* Actor, class UObject* SubObject, class UNetConnection* Connection, class UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack, bool IsServer);

public:

	/** Used to specify the class to use for connections */
	UPROPERTY(Config)
	FString NetConnectionClassName;

	UPROPERTY(Config)
	FString ReplicationDriverClassName;

	/** @todo document */
	UPROPERTY(Config)
	int32 MaxDownloadSize;

	/** @todo document */
	UPROPERTY(Config)
	uint32 bClampListenServerTickRate:1;

	/** @todo document */
	UPROPERTY(Config)
	int32 NetServerMaxTickRate;

	/** @todo document */
	UPROPERTY(Config)
	int32 MaxInternetClientRate;

	/** @todo document */
	UPROPERTY(Config)
	int32 MaxClientRate;

	/** Amount of time a server will wait before traveling to next map, gives clients time to receive final RPCs on existing level @see NextSwitchCountdown */
	UPROPERTY(Config)
	float ServerTravelPause;

	/** @todo document */
	UPROPERTY(Config)
	float SpawnPrioritySeconds;

	/** @todo document */
	UPROPERTY(Config)
	float RelevantTimeout;

	/** @todo document */
	UPROPERTY(Config)
	float KeepAliveTime;

	/** Amount of time to wait for a new net connection to be established before destroying the connection */
	UPROPERTY(Config)
	float InitialConnectTimeout;

	/** 
	 * Amount of time to wait before considering an established connection timed out.  
	 * Typically shorter than the time to wait on a new connection because this connection
	 * should already have been setup and any interruption should be trapped quicker.
	 */
	UPROPERTY(Config)
	float ConnectionTimeout;

	/**
	* A multiplier that is applied to the above values when we are running with unoptimized builds (debug)
	* or data (uncooked). This allows us to retain normal timeout behavior while debugging without resorting
	* to the nuclear 'notimeouts' option or bumping the values above. If ==0 multiplier = 1
	*/
	UPROPERTY(Config)
	float TimeoutMultiplierForUnoptimizedBuilds;

	/**
	 * If true, ignore timeouts completely.  Should be used only in development
	 */
	UPROPERTY(Config)
	bool bNoTimeouts;

	/** Connection to the server (this net driver is a client) */
	UPROPERTY()
	class UNetConnection* ServerConnection;

	/** Array of connections to clients (this net driver is a host) - unsorted, and ordering changes depending on actor replication */
	UPROPERTY()
	TArray<UNetConnection*> ClientConnections;

	/**
	 * Map of IP's to NetConnection's - for fast lookup, particularly under DDoS.
	 * Only valid IP's mapped (e.g. excludes DemoNetConnection). Recently disconnected clients remain mapped as nullptr connections.
	 */
	FConnectionMap MappedClientConnections;

	/** Tracks recently disconnected client IP's, and the disconnect time - so they can be cleaned from MappedClientConnections */
	TArray<FDisconnectedClient> RecentlyDisconnectedClients;

	/** The amount of time, in seconds, that recently disconnected clients should be tracked */
	UPROPERTY(Config)
	int32 RecentlyDisconnectedTrackingTime;


	/** Serverside PacketHandler for managing connectionless packets */
	TUniquePtr<PacketHandler> ConnectionlessHandler;

	/** Reference to the PacketHandler component, for managing stateless connection handshakes */
	TWeakPtr<StatelessConnectHandlerComponent> StatelessConnectComponent;

	/** The analytics provider used by the packet handler */
	TSharedPtr<IAnalyticsProvider> AnalyticsProvider;

	/** Special analytics aggregator tied to AnalyticsProvider - combines analytics from all NetConnections/PacketHandlers, in one event */
	TSharedPtr<FNetAnalyticsAggregator> AnalyticsAggregator;

	/** World this net driver is associated with */
	UPROPERTY()
	class UWorld* World;

	UPROPERTY()
	class UPackage* WorldPackage;

	/** @todo document */
	TSharedPtr< class FNetGUIDCache > GuidCache;

	TSharedPtr< class FClassNetCacheMgr >	NetCache;

	/** The loaded UClass of the net connection type to use */
	UPROPERTY()
	UClass* NetConnectionClass;

	UPROPERTY()
	UClass* ReplicationDriverClass;

	/** @todo document */
	UPROPERTY()
	UProperty* RoleProperty;
	
	/** @todo document */
	UPROPERTY()
	UProperty* RemoteRoleProperty;

	/** Used to specify the net driver to filter actors with (NAME_None || NAME_GameNetDriver is the default net driver) */
	UPROPERTY(Config)
	FName NetDriverName;

	/** The UChannel classes that should be used under this net driver */
	UE_DEPRECATED(4.22, "Use ChannelDefinitions instead")
	UClass* ChannelClasses[CHTYPE_MAX];

	/** Used to specify available channel types and their associated UClass */
	UPROPERTY(Config)
	TArray<FChannelDefinition> ChannelDefinitions;

	/** Used for faster lookup of channel definitions by name. */
	UPROPERTY()
	TMap<FName, FChannelDefinition> ChannelDefinitionMap;

	/** @return true if the specified channel type exists. */
	UE_DEPRECATED(4.22, "Use IsKnownChannelName")
	bool IsKnownChannelType(int32 Type);

	/** @return true if the specified channel definition exists. */
	FORCEINLINE bool IsKnownChannelName(const FName& ChName)
	{
		return ChannelDefinitionMap.Contains(ChName);
	}

	/** Creates a channel of each type that is set as bInitialClient. */
	ENGINE_API void CreateInitialClientChannels();

	/** Creates a channel of each type that is set as bIniitalServer for the given connection. */
	ENGINE_API void CreateInitialServerChannels(UNetConnection* ClientConnection);

private:

	/** List of channels that were previously used and can be used again */
	UPROPERTY()
	TArray<UChannel*> ActorChannelPool;

public:

	/** Creates a new channel of the specified type. If the type is pooled, it will return a pre-created channel */
	UE_DEPRECATED(4.22, "Use GetOrCreateChannelByName")
	UChannel* GetOrCreateChannel(EChannelType ChType);

	/** Creates a new channel of the specified type name. If the type is pooled, it will return a pre-created channel */
	UChannel* GetOrCreateChannelByName(const FName& ChName);

	/** If the channel's type is pooled, this will add the channel to the pool. Otherwise, nothing will happen. */
	void ReleaseToChannelPool(UChannel* Channel);

	/** Change the NetDriver's NetDriverName. This will also reinit packet simulation settings so that settings can be qualified to a specific driver. */
	void SetNetDriverName(FName NewNetDriverNamed);

	void InitPacketSimulationSettings();

	/** Returns true during the duration of a packet loss burst triggered by the net.pktlossburst command. */
	bool IsSimulatingPacketLossBurst() const;

	/** Interface for communication network state to others (ie World usually, but anything that implements FNetworkNotify) */
	class FNetworkNotify*		Notify;
	
	/** Accumulated time for the net driver, updated by Tick */
	UPROPERTY()
	float						Time;

	/** Last realtime a tick dispatch occurred. Used currently to try and diagnose timeout issues */
	double						LastTickDispatchRealtime;

	/** If true then client connections are to other client peers */
	bool						bIsPeer;
	/** @todo document */
	bool						ProfileStats;
	/** If true, it assumes the stats are being set by server data */
	bool						bSkipLocalStats;
	/** Timings for Socket::SendTo() */
	int32						SendCycles;
	/** Stats for network perf */
	uint32						InBytesPerSecond;
	/** todo document */
	uint32						OutBytesPerSecond;
	/** todo document */
	uint32						InBytes;
	/** Total bytes in packets received since the net driver's creation */
	uint32						InTotalBytes;
	/** todo document */
	uint32						OutBytes;
	/** Total bytes in packets sent since the net driver's creation */
	uint32						OutTotalBytes;
	/** Outgoing rate of NetGUID Bunches */
	uint32						NetGUIDOutBytes;
	/** Incoming rate of NetGUID Bunches */
	uint32						NetGUIDInBytes;
	/** todo document */
	uint32						InPackets;
	/** Total packets received since the net driver's creation  */
	uint32						InTotalPackets;
	/** todo document */
	uint32						OutPackets;
	/** Total packets sent since the net driver's creation  */
	uint32						OutTotalPackets;
	/** todo document */
	uint32						InBunches;
	/** todo document */
	uint32						OutBunches;
	/** Total bunches received since the net driver's creation  */
	uint32						InTotalBunches;
	/** Total bunches sent since the net driver's creation  */
	uint32						OutTotalBunches;
	/** todo document */
	uint32						InPacketsLost;
	/** Total packets lost that have been sent by clients since the net driver's creation  */
	uint32						InTotalPacketsLost;
	/** todo document */
	uint32						OutPacketsLost;
	/** Total packets lost that have been sent by the server since the net driver's creation  */
	uint32						OutTotalPacketsLost;
	/** todo document */
	uint32						InOutOfOrderPackets;
	/** todo document */
	uint32						OutOutOfOrderPackets;
	/** Tracks the total number of voice packets sent */
	uint32						VoicePacketsSent;
	/** Tracks the total number of voice bytes sent */
	uint32						VoiceBytesSent;
	/** Tracks the total number of voice packets received */
	uint32						VoicePacketsRecv;
	/** Tracks the total number of voice bytes received */
	uint32						VoiceBytesRecv;
	/** Tracks the voice data percentage of in bound bytes */
	uint32						VoiceInPercent;
	/** Tracks the voice data percentage of out bound bytes */
	uint32						VoiceOutPercent;
	/** Time of last stat update */
	double						StatUpdateTime;
	/** Interval between gathering stats */
	float						StatPeriod;
	/** Total RPCs called since the net driver's creation  */
	uint32						TotalRPCsCalled;
	/** Total acks sent since the net driver's creation  */
	uint32						OutTotalAcks;

	/** Collect net stats even if not FThreadStats::IsCollectingData(). */
	bool bCollectNetStats;
	/** Time of last netdriver cleanup pass */
	double						LastCleanupTime;
	/** Used to determine if checking for standby cheats should occur */
	bool						bIsStandbyCheckingEnabled;
	/** Used to determine whether we've already caught a cheat or not */
	bool						bHasStandbyCheatTriggered;
	/** The amount of time without packets before triggering the cheat code */
	float						StandbyRxCheatTime;
	/** todo document */
	float						StandbyTxCheatTime;
	/** The point we think the host is cheating or shouldn't be hosting due to crappy network */
	int32						BadPingThreshold;
	/** The number of clients missing data before triggering the standby code */
	float						PercentMissingForRxStandby;
	float						PercentMissingForTxStandby;
	/** The number of clients with bad ping before triggering the standby code */
	float						PercentForBadPing;
	/** The amount of time to wait before checking a connection for standby issues */
	float						JoinInProgressStandbyWaitTime;
	/** Used to track whether a given actor was replicated by the net driver recently */
	int32						NetTag;
	/** Dumps next net update's relevant actors when true*/
	bool						DebugRelevantActors;

	/** These are debug list of actors. They are using TWeakObjectPtr so that they do not affect GC performance since they are rarely in use (DebugRelevantActors) */
	TArray< TWeakObjectPtr<AActor> >	LastPrioritizedActors;
	TArray< TWeakObjectPtr<AActor> >	LastRelevantActors;
	TArray< TWeakObjectPtr<AActor> >	LastSentActors;
	TArray< TWeakObjectPtr<AActor> >	LastNonRelevantActors;

	void						PrintDebugRelevantActors();
	
	/** The server adds an entry into this map for every actor that is destroyed that join-in-progress
	 *  clients need to know about, that is, startup actors. Also, individual UNetConnections
	 *  need to keep track of FActorDestructionInfo for dormant and recently-dormant actors in addition
	 *  to startup actors (because they won't have an associated channel), and this map stores those
	 *  FActorDestructionInfos also.
	 */
	TMap<FNetworkGUID, TUniquePtr<FActorDestructionInfo>>	DestroyedStartupOrDormantActors;

	/** The server adds an entry into this map for every startup actor that has been renamed, and will
	 *  always map from current name to original name
	 */
	TMap<FName, FName>	RenamedStartupActors;

	class FRepChangedPropertyTrackerWrapper
	{
	public:
		FRepChangedPropertyTrackerWrapper(UObject* Obj, const TSharedPtr<FRepChangedPropertyTracker>& InRepChangedPropertyTracker) : RepChangedPropertyTracker(InRepChangedPropertyTracker), WeakObjectPtr(Obj) {}

		const FRepChangedPropertyTracker* operator->() const { return RepChangedPropertyTracker.Get(); }
		FRepChangedPropertyTracker* operator->() { return RepChangedPropertyTracker.Get(); }
		
		const FRepChangedPropertyTracker* Get() const { return RepChangedPropertyTracker.Get(); }
		FRepChangedPropertyTracker* Get() { return RepChangedPropertyTracker.Get(); }

		bool IsValid() const { return RepChangedPropertyTracker.IsValid(); }
		bool IsObjectValid() const { return WeakObjectPtr.IsValid(); }

		TWeakObjectPtr<UObject> GetWeakObjectPtr() const { return WeakObjectPtr; }

		TSharedPtr<FRepChangedPropertyTracker> RepChangedPropertyTracker;

		void CountBytes(FArchive& Ar) const;

	private:
		TWeakObjectPtr<UObject> WeakObjectPtr;
	};
	/** Maps FRepChangedPropertyTracker to active objects that are replicating properties */
	TMap<UObject*, FRepChangedPropertyTrackerWrapper>	RepChangedPropertyTrackerMap;
	/** Used to invalidate properties marked "unchanged" in FRepChangedPropertyTracker's */
	uint32																		ReplicationFrame;

	/** Maps FRepLayout to the respective UClass */
	TMap< TWeakObjectPtr< UObject >, TSharedPtr< FRepLayout > >					RepLayoutMap;

	class FReplicationChangelistMgrWrapper
	{
	public:
		FReplicationChangelistMgrWrapper(UObject* Obj, const TSharedPtr<FReplicationChangelistMgr>& InReplicationChangelistMgr) : ReplicationChangelistMgr(InReplicationChangelistMgr), WeakObjectPtr(Obj) {}

		const FReplicationChangelistMgr* operator->() const { return ReplicationChangelistMgr.Get(); }
		FReplicationChangelistMgr* operator->() { return ReplicationChangelistMgr.Get(); }

		bool IsValid() const { return ReplicationChangelistMgr.IsValid(); }
		bool IsObjectValid() const { return WeakObjectPtr.IsValid(); }

		TWeakObjectPtr<UObject> GetWeakObjectPtr() const { return WeakObjectPtr; }

		TSharedPtr<FReplicationChangelistMgr> ReplicationChangelistMgr;

		void CountBytes(FArchive& Ar) const;

	private:
		TWeakObjectPtr<UObject> WeakObjectPtr;
	};
	/** Maps an object to the respective FReplicationChangelistMgr */
	TMap< UObject*, FReplicationChangelistMgrWrapper >	ReplicationChangeListMap;

	/** Creates if necessary, and returns a FRepLayout that maps to the passed in UClass */
	TSharedPtr< FRepLayout >	GetObjectClassRepLayout( UClass * InClass );

	/** Creates if necessary, and returns a FRepLayout that maps to the passed in UFunction */
	ENGINE_API TSharedPtr<FRepLayout> GetFunctionRepLayout( UFunction * Function );

	/** Creates if necessary, and returns a FRepLayout that maps to the passed in UStruct */
	TSharedPtr<FRepLayout>		GetStructRepLayout( UStruct * Struct );

	/**
	 * Returns the FReplicationChangelistMgr that is associated with the passed in object,
	 * creating one if none exist.
	 *
	 * This should **never** be called on client NetDrivers!
	 */
	TSharedPtr< FReplicationChangelistMgr > GetReplicationChangeListMgr( UObject* Object );

	TMap< FNetworkGUID, TSet< FObjectReplicator* > >	GuidToReplicatorMap;
	int32												TotalTrackedGuidMemoryBytes;
	TSet< FObjectReplicator* >							UnmappedReplicators;
	TSet< FObjectReplicator* >							AllOwnedReplicators;

	/** Handles to various registered delegates */
	FDelegateHandle TickDispatchDelegateHandle;
	FDelegateHandle PostTickDispatchDelegateHandle;
	FDelegateHandle TickFlushDelegateHandle;
	FDelegateHandle PostTickFlushDelegateHandle;

#if !UE_BUILD_SHIPPING
	/** Delegate for hooking ProcessRemoteFunction */
	FOnSendRPC	SendRPCDel;
#endif

	/** Tracks the amount of time spent during the current frame processing queued bunches. */
	float ProcessQueuedBunchesCurrentFrameMilliseconds;

	/** DDoS detection management */
	FDDoSDetection DDoS;



	/**
	* Updates the standby cheat information and
	 * causes the dialog to be shown/hidden as needed
	 */
	void UpdateStandbyCheatStatus(void);

	/** Sets the analytics provider */
	void ENGINE_API SetAnalyticsProvider(TSharedPtr<IAnalyticsProvider> InProvider);

#if DO_ENABLE_NET_TEST
	FPacketSimulationSettings	PacketSimulationSettings;

	ENGINE_API void SetPacketSimulationSettings(FPacketSimulationSettings NewSettings);
#endif

	// Constructors.
	ENGINE_API UNetDriver(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	//~ Begin UObject Interface.
	ENGINE_API virtual void PostInitProperties() override;
	ENGINE_API virtual void PostReloadConfig(UProperty* PropertyToLoad) override;
	ENGINE_API virtual void FinishDestroy() override;
	ENGINE_API virtual void Serialize( FArchive& Ar ) override;
	ENGINE_API static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	//~ End UObject Interface.

	//~ Begin FExec Interface

	/**
	 * Handle exec commands
	 *
	 * @param InWorld	the world context
	 * @param Cmd		the exec command being executed
	 * @param Ar		the archive to log results to
	 *
	 * @return true if the handler consumed the input, false to continue searching handlers
	 */
	ENGINE_API virtual bool Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar=*GLog) override;

	ENGINE_API ENetMode	GetNetMode() const;

	//~ End FExec Interface.

	/** 
	 * Returns true if this net driver is valid for the current configuration.
	 * Safe to call on a CDO if necessary
	 *
	 * @return true if available, false otherwise
	 */
	ENGINE_API virtual bool IsAvailable() const PURE_VIRTUAL( UNetDriver::IsAvailable, return false;)

	/**
	 * Common initialization between server and client connection setup
	 * 
	 * @param bInitAsClient are we a client or server
	 * @param InNotify notification object to associate with the net driver
	 * @param URL destination
	 * @param bReuseAddressAndPort whether to allow multiple sockets to be bound to the same address/port
	 * @param Error output containing an error string on failure
	 *
	 * @return true if successful, false otherwise (check Error parameter)
	 */
	ENGINE_API virtual bool InitBase(bool bInitAsClient, FNetworkNotify* InNotify, const FURL& URL, bool bReuseAddressAndPort, FString& Error);

	/**
	 * Initialize the net driver in client mode
	 *
	 * @param InNotify notification object to associate with the net driver
	 * @param ConnectURL remote ip:port of host to connect to
	 * @param Error resulting error string from connection attempt
	 * 
	 * @return true if successful, false otherwise (check Error parameter)
	 */
	ENGINE_API virtual bool InitConnect(class FNetworkNotify* InNotify, const FURL& ConnectURL, FString& Error ) PURE_VIRTUAL( UNetDriver::InitConnect, return true;);

	/**
	 * Initialize the network driver in server mode (listener)
	 *
	 * @param InNotify notification object to associate with the net driver
	 * @param ListenURL the connection URL for this listener
	 * @param bReuseAddressAndPort whether to allow multiple sockets to be bound to the same address/port
	 * @param Error out param with any error messages generated 
	 *
	 * @return true if successful, false otherwise (check Error parameter)
	 */
	ENGINE_API virtual bool InitListen(class FNetworkNotify* InNotify, FURL& ListenURL, bool bReuseAddressAndPort, FString& Error) PURE_VIRTUAL( UNetDriver::InitListen, return true;);

	/** Initialize the list of destroyed net startup actors from the current World */
	ENGINE_API virtual void InitDestroyedStartupActors();

	/**
	 * Initialize a PacketHandler for serverside net drivers, for handling connectionless packets
	 * NOTE: Only triggered by net driver subclasses that support it - from within InitListen.
	 */
	ENGINE_API virtual void InitConnectionlessHandler();

	/**
	 * Flushes all packets queued by the connectionless PacketHandler
	 * NOTE: This should be called shortly after all calls to PacketHandler::IncomingConnectionless, to minimize packet buffer buildup.
	 */
	ENGINE_API virtual void FlushHandler();


	/** Initializes the net connection class to use for new connections */
	ENGINE_API virtual bool InitConnectionClass(void);

	/** Initialized the replication driver class to use for this driver */
	ENGINE_API virtual bool InitReplicationDriverClass();

	/** Shutdown all connections managed by this net driver */
	ENGINE_API virtual void Shutdown();

	/* Close socket and Free the memory the OS allocated for this socket */
	ENGINE_API virtual void LowLevelDestroy();

	/* @return network number */
	virtual FString LowLevelGetNetworkNumber() PURE_VIRTUAL(UNetDriver::LowLevelGetNetworkNumber,return TEXT(""););

	/** Make sure this connection is in a reasonable state. */
	ENGINE_API virtual void AssertValid();

	/**
	 * Called to replicate any relevant actors to the connections contained within this net driver
	 *
	 * Process as many clients as allowed given Engine.NetClientTicksPerSecond, first building a list of actors to consider for relevancy checking,
	 * and then attempting to replicate each actor for each connection that it is relevant to until the connection becomes saturated.
	 *
	 * NetClientTicksPerSecond is used to throttle how many clients are updated each frame, hoping to avoid saturating the server's upstream bandwidth, although
	 * the current solution is far from optimal.  Ideally the throttling could be based upon the server connection becoming saturated, at which point each
	 * connection is reduced to priority only updates, and spread out amongst several ticks.  Also might want to investigate eliminating the redundant consider/relevancy
	 * checks for Actors that were successfully replicated for some channels but not all, since that would make a decent CPU optimization.
	 *
	 * @param DeltaSeconds elapsed time since last call
	 *
	 * @return the number of actors that were replicated
	 */
	ENGINE_API virtual int32 ServerReplicateActors(float DeltaSeconds);

	/**
	 * Process a remote function call on some actor destined for a remote location
	 *
	 * @param Actor actor making the function call
	 * @param Function function definition called
	 * @param Params parameters in a UObject memory layout
	 * @param Stack stack frame the UFunction is called in
	 * @param SubObject optional: sub object to actually call function on
	 */
	ENGINE_API virtual void ProcessRemoteFunction(class AActor* Actor, class UFunction* Function, void* Parameters, struct FOutParmRec* OutParms, struct FFrame* Stack, class UObject* SubObject = nullptr );


	enum ENGINE_API ERemoteFunctionSendPolicy
	{		
		/** Unreliable multicast are queued. Everything else is send immediately */
		Default, 

		/** Bunch is send immediately no matter what */
		ForceSend,

		/** Bunch is queued until next actor replication, no matter what */
		ForceQueue,
	};	

	/** Process a remote function on given actor channel. This is called by ::ProcessRemoteFunction.*/
	ENGINE_API void ProcessRemoteFunctionForChannel(UActorChannel* Ch, const class FClassNetCache* ClassCache, const FFieldNetCache* FieldCache, UObject* TargetObj, UNetConnection* Connection,  UFunction* Function, void* Parms, FOutParmRec* OutParms, FFrame* Stack, const bool IsServer, const ERemoteFunctionSendPolicy SendPolicy = ERemoteFunctionSendPolicy::Default);

	/** handle time update: read and process packets */
	ENGINE_API virtual void TickDispatch( float DeltaTime );

	/** PostTickDispatch actions */
	ENGINE_API virtual void PostTickDispatch();

	/** ReplicateActors and Flush */
	ENGINE_API virtual void TickFlush(float DeltaSeconds);

	/** PostTick actions */
	ENGINE_API virtual void PostTickFlush();

	UE_DEPRECATED(4.21, "Please use the LowLevelSend that requires packet traits for analytics and packet modifiers.")
	ENGINE_API virtual void LowLevelSend(FString Address, void* Data, int32 CountBits)
	{
		FOutPacketTraits EmptyTraits;
		LowLevelSend(Address, Data, CountBits, EmptyTraits);
	}

	/**
	 * Sends a 'connectionless' (not associated with a UNetConection) packet, to the specified address.
	 * NOTE: Address is an abstract format defined by subclasses. Anything calling this, must use an address supplied by the net driver.
	 *
	 * @param Address		The address the packet should be sent to (format is abstract, determined by net driver subclasses)
	 * @param Data			The packet data
	 * @param CountBits		The size of the packet data, in bits
	 * @param Traits		Traits for the packet, if applicable
	 */
	ENGINE_API virtual void LowLevelSend(FString Address, void* Data, int32 CountBits, FOutPacketTraits& Traits)
		PURE_VIRTUAL(UNetDriver::LowLevelSend,);

	/**
	 * Process any local talker packets that need to be sent to clients
	 */
	ENGINE_API virtual void ProcessLocalServerPackets();

	/**
	 * Process any local talker packets that need to be sent to the server
	 */
	ENGINE_API virtual void ProcessLocalClientPackets();

	/**
	 * Update the LagState based on a heuristic to determine if we are network lagging
	 */
	ENGINE_API virtual void UpdateNetworkLagState();

	/**
	 * Determines which other connections should receive the voice packet and
	 * queues the packet for those connections. Used for sending both local/remote voice packets.
	 *
	 * @param VoicePacket the packet to be queued
	 * @param CameFromConn the connection this packet came from (NULL if local)
	 */
	ENGINE_API virtual void ReplicateVoicePacket(TSharedPtr<class FVoicePacket> VoicePacket, class UNetConnection* CameFromConn);

#if !UE_BUILD_SHIPPING
	/**
	 * Exec command handlers
	 */
	bool HandleSocketsCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandlePackageMapCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleNetFloodCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleNetDebugTextCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleNetDisconnectCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleNetDumpServerRPCCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleNetDumpDormancy( const TCHAR* Cmd, FOutputDevice& Ar );
#endif

	void HandlePacketLossBurstCommand( int32 DurationInMilliseconds );

	// ---------------------------------------------------------------
	//	Game code API for updating server Actor Replication State
	// ---------------------------------------------------------------

	ENGINE_API virtual void ForceNetUpdate(AActor* Actor);

	/** Flushes actor from NetDriver's dormancy list, but does not change any state on the Actor itself */
	ENGINE_API void FlushActorDormancy(AActor *Actor, bool bWasDormInitial=false);

	ENGINE_API void NotifyActorDormancyChange(AActor* Actor, ENetDormancy OldDormancyState);

	/** Forces properties on this actor to do a compare for one frame (rather than share shadow state) */
	ENGINE_API void ForcePropertyCompare( AActor* Actor );

	/** Force this actor to be relevant for at least one update */
	ENGINE_API void ForceActorRelevantNextUpdate(AActor* Actor);

	/** Tells the net driver about a networked actor that was spawned */
	ENGINE_API void AddNetworkActor(AActor* Actor);

	/** Called when a spawned actor is destroyed. */
	ENGINE_API virtual void NotifyActorDestroyed( AActor* Actor, bool IsSeamlessTravel=false );

	/** Called when an actor is renamed. */
	ENGINE_API virtual void NotifyActorRenamed(AActor* Actor, FName PreviousName);

	ENGINE_API void RemoveNetworkActor(AActor* Actor);

	ENGINE_API virtual void NotifyActorLevelUnloaded( AActor* Actor );

	ENGINE_API virtual void NotifyActorTearOff(AActor* Actor);

	// ---------------------------------------------------------------
	//
	// ---------------------------------------------------------------	

	ENGINE_API virtual void NotifyStreamingLevelUnload( ULevel* );

	/** creates a child connection and adds it to the given parent connection */
	ENGINE_API virtual class UChildConnection* CreateChild(UNetConnection* Parent);

	/** @return String that uniquely describes the net driver instance */
	FString GetDescription() 
	{ 
		return FString::Printf(TEXT("%s %s%s"), *NetDriverName.ToString(), *GetName(), bIsPeer ? TEXT("(PEER)") : TEXT(""));
	}

	/** @return true if this netdriver is handling accepting connections */
	ENGINE_API virtual bool IsServer() const;

	ENGINE_API virtual void CleanPackageMaps();

	void RemoveClassRepLayoutReferences(UClass* Class);

	ENGINE_API void CleanupWorldForSeamlessTravel();

	ENGINE_API void PreSeamlessTravelGarbageCollect();

	ENGINE_API void PostSeamlessTravelGarbageCollect();

	/**
	 * Get the socket subsytem appropriate for this net driver
	 */
	virtual class ISocketSubsystem* GetSocketSubsystem() PURE_VIRTUAL(UNetDriver::GetSocketSubsystem, return NULL;);

	/**
	 * Associate a world with this net driver. 
	 * Disassociates any previous world first.
	 * 
	 * @param InWorld the world to associate with this netdriver
	 */
	ENGINE_API void SetWorld(class UWorld* InWorld);

	/**
	 * Get the world associated with this net driver
	 */
	virtual class UWorld* GetWorld() const override final { return World; }

	class UPackage* GetWorldPackage() const { return WorldPackage; }

	/** Called during seamless travel to clear all state that was tied to the previous game world (actor lists, etc) */
	ENGINE_API virtual void ResetGameWorldState();

	/** @return true if the net resource is valid or false if it should not be used */
	virtual bool IsNetResourceValid(void) PURE_VIRTUAL(UNetDriver::IsNetResourceValid, return false;);

	bool NetObjectIsDynamic(const UObject *Object) const;

	/** Draws debug markers in the world based on network state */
	void DrawNetDriverDebug();

	/** 
	 * Finds a FRepChangedPropertyTracker associated with an object.
	 * If not found, creates one.
	*/
	TSharedPtr<FRepChangedPropertyTracker> FindOrCreateRepChangedPropertyTracker(UObject *Obj);

	/** Returns true if the client should destroy immediately any actor that becomes torn-off */
	virtual bool ShouldClientDestroyTearOffActors() const { return false; }

	/** Returns whether or not properties that are replicating using this driver should not call RepNotify functions. */
	virtual bool ShouldSkipRepNotifies() const { return false; }

	/** Returns true if actor channels with InGUID should queue up bunches, even if they wouldn't otherwise be queued. */
	virtual bool ShouldQueueBunchesForActorGUID(FNetworkGUID InGUID) const { return false; }

	/** Returns whether or not RPCs processed by this driver should be ignored. */
	virtual bool ShouldIgnoreRPCs() const { return false; }

	/** Returns the existing FNetworkGUID of InActor, if it has one. */
	virtual FNetworkGUID GetGUIDForActor(const AActor* InActor) const { return FNetworkGUID(); }

	/** Returns the actor that corresponds to InGUID, if one can be found. */
	virtual AActor* GetActorForGUID(FNetworkGUID InGUID) const { return nullptr; }

	/** Returns true if RepNotifies should be checked and generated when receiving properties for the given object. */
	virtual bool ShouldReceiveRepNotifiesForObject(UObject* Object) const { return true; }

	/** Returns the object that manages the list of replicated UObjects. */
	ENGINE_API FNetworkObjectList& GetNetworkObjectList() { return *NetworkObjects; }

	/** Returns the object that manages the list of replicated UObjects. */
	ENGINE_API const FNetworkObjectList& GetNetworkObjectList() const { return *NetworkObjects; }

	/**
     *	Get the network object matching the given Actor.
	 *	If the Actor is not present in the NetworkObjectInfo list, it will be added.
	 */
	ENGINE_API FNetworkObjectInfo* FindOrAddNetworkObjectInfo(const AActor* InActor);

	/** Get the network object matching the given Actor. */
	ENGINE_API FNetworkObjectInfo* FindNetworkObjectInfo(const AActor* InActor);
	ENGINE_API const FNetworkObjectInfo* FindNetworkObjectInfo(const AActor* InActor) const
	{
		return const_cast<UNetDriver*>(this)->FindNetworkObjectInfo(InActor);
	}

	/**
	 * Returns whether adaptive net frequency is enabled. If enabled, update frequency is allowed to ramp down to MinNetUpdateFrequency for an actor when no replicated properties have changed.
	 * This is currently controlled by the CVar "net.UseAdaptiveNetUpdateFrequency".
	 */
	ENGINE_API static bool IsAdaptiveNetUpdateFrequencyEnabled();

	/** Returns true if adaptive net update frequency is enabled and the given actor is having its update rate lowered from its standard rate. */
	ENGINE_API bool IsNetworkActorUpdateFrequencyThrottled(const AActor* InActor) const;

	/** Returns true if adaptive net update frequency is enabled and the given actor is having its update rate lowered from its standard rate. */
	ENGINE_API bool IsNetworkActorUpdateFrequencyThrottled(const FNetworkObjectInfo& InNetworkActor) const;

	/** Stop adaptive replication for the given actor if it's currently throttled. It maybe be allowed to throttle again later. */
	ENGINE_API void CancelAdaptiveReplication(FNetworkObjectInfo& InNetworkActor);

	/** Returns the level ID/PIE instance ID for this netdriver to use. */
	ENGINE_API int32 GetDuplicateLevelID() const { return DuplicateLevelID; }

	/** Sets the level ID/PIE instance ID for this netdriver to use. */
	ENGINE_API void SetDuplicateLevelID(const int32 InDuplicateLevelID) { DuplicateLevelID = InDuplicateLevelID; }

	/** Explicitly sets the ReplicationDriver instance (you instantiate it and initialize it). Shouldn't be done during gameplay: ok to do in GameMode startup or via console commands for testing. Existing ReplicationDriver (if set) is destroyed when this is called.  */
	ENGINE_API void SetReplicationDriver(UReplicationDriver* NewReplicationManager);

	ENGINE_API UReplicationDriver* GetReplicationDriver() { return ReplicationDriver; }

	template<class T>
	T* GetReplicationDriver() { return Cast<T>(ReplicationDriver); }

	void RemoveClientConnection(UNetConnection* ClientConnectionToRemove);

	/** Adds (fully initialized, ready to go) client connection to the ClientConnections list + any other game related setup */
	ENGINE_API void	AddClientConnection(UNetConnection * NewConnection);

	ENGINE_API void NotifyActorFullyDormantForConnection(AActor* Actor, UNetConnection* Connection);

	/** Returns true if this actor is considered to be in a loaded level */
	ENGINE_API virtual bool IsLevelInitializedForActor( const AActor* InActor, const UNetConnection* InConnection ) const;

	/** Called after processing RPC to track time spent */
	void NotifyRPCProcessed(UFunction* Function, UNetConnection* Connection, double ElapsedTimeSeconds);

	/** Returns true if this network driver will handle the remote function call for the given actor. */
	ENGINE_API virtual bool ShouldReplicateFunction(AActor* Actor, UFunction* Function) const;

	/** Returns true if this network driver will forward a received remote function call to other active net drivers. */
	ENGINE_API virtual bool ShouldForwardFunction(AActor* Actor, UFunction* Function, void* Parms) const;

	/** Returns true if this network driver will replicate the given actor. */
	ENGINE_API virtual bool ShouldReplicateActor(AActor* Actor) const;

	/** Returns true if this network driver should execute this remote call locally. */
	ENGINE_API virtual bool ShouldCallRemoteFunction(UObject* Object, UFunction* Function, const FReplicationFlags& RepFlags) const;

	/** Returns true if clients should destroy the actor when the channel is closed. */
	ENGINE_API virtual bool ShouldClientDestroyActor(AActor* Actor) const;

	/** Called when an actor channel is remotely opened for an actor. */
	ENGINE_API virtual void NotifyActorChannelOpen(UActorChannel* Channel, AActor* Actor) {}

protected:

	/** Register all TickDispatch, TickFlush, PostTickFlush to tick in World */
	ENGINE_API void RegisterTickEvents(class UWorld* InWorld);
	/** Unregister all TickDispatch, TickFlush, PostTickFlush to tick in World */
	ENGINE_API void UnregisterTickEvents(class UWorld* InWorld);

	UE_DEPRECATED(4.22, "Use InternalCreateChannelByName")
	/** Subclasses may override this to customize channel creation. Called by GetOrCreateChannel if the pool is exhausted and a new channel must be allocated. */
	ENGINE_API virtual UChannel* InternalCreateChannel(EChannelType ChType);

	/** Subclasses may override this to customize channel creation. Called by GetOrCreateChannel if the pool is exhausted and a new channel must be allocated. */
	ENGINE_API virtual UChannel* InternalCreateChannelByName(const FName& ChName);

#if WITH_SERVER_CODE
	/**
	* Helper functions for ServerReplicateActors
	*/
	int32 ServerReplicateActors_PrepConnections( const float DeltaSeconds );
	void ServerReplicateActors_BuildConsiderList( TArray<FNetworkObjectInfo*>& OutConsiderList, const float ServerTickTime );
	int32 ServerReplicateActors_PrioritizeActors( UNetConnection* Connection, const TArray<FNetViewer>& ConnectionViewers, const TArray<FNetworkObjectInfo*> ConsiderList, const bool bCPUSaturated, FActorPriority*& OutPriorityList, FActorPriority**& OutPriorityActors );
	int32 ServerReplicateActors_ProcessPrioritizedActors( UNetConnection* Connection, const TArray<FNetViewer>& ConnectionViewers, FActorPriority** PriorityActors, const int32 FinalSortedCount, int32& OutUpdated );
#endif

	/** Used to handle any NetDriver specific cleanup once a level has been removed from the world. */
	ENGINE_API virtual void OnLevelRemovedFromWorld(class ULevel* Level, class UWorld* World);

	/** Used to handle any NetDriver specific setup when a level has been added to the world. */
	ENGINE_API virtual void OnLevelAddedToWorld(class ULevel* Level, class UWorld* World);

	/** Handles that track our LevelAdded / Removed delegates. */
	FDelegateHandle OnLevelRemovedFromWorldHandle;
	FDelegateHandle OnLevelAddedToWorldHandle;

private:
	FActorDestructionInfo* CreateDestructionInfo(UNetDriver* NetDriver, AActor* ThisActor, FActorDestructionInfo *DestructionInfo);

	void CreateReplicatedStaticActorDestructionInfo(UNetDriver* NetDriver, ULevel* Level, const FReplicatedStaticActorDestructionInfo& Info);


	void FlushActorDormancyInternal(AActor *Actor);

	void LoadChannelDefinitions();

	UPROPERTY(transient)
	UReplicationDriver* ReplicationDriver;

	/** Stores the list of objects to replicate into the replay stream. This should be a TUniquePtr, but it appears the generated.cpp file needs the full definition of the pointed-to type. */
	TSharedPtr<FNetworkObjectList> NetworkObjects;

	/** Set to "Lagging" on the server when all client connections are near timing out. We are lagging on the client when the server connection is near timed out. */
	ENetworkLagState::Type LagState;

	/** Duplicate level instance to use for playback (PIE instance ID) */
	int32 DuplicateLevelID;

	/** NetDriver time to end packet loss burst simulation. */
	float PacketLossBurstEndTime;
};
