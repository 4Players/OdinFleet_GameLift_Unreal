// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameLiftExeptions.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GLBSServiceConnector.generated.h"

/**
 * 
 */


USTRUCT(BlueprintType,Blueprintable)
struct FSessionPlacementData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString GameSessionName;

	UPROPERTY(BlueprintReadWrite)
	FString GameSessionQueueName;
	
	UPROPERTY(BlueprintReadWrite)
	int32 MaximumPlayerSessionCount;

	UPROPERTY(BlueprintReadWrite)
	FString PlacementId;

	UPROPERTY(BlueprintReadWrite)
	FString Status;

	UPROPERTY(BlueprintReadWrite)
	FDateTime StartTime;
	
};
USTRUCT(BlueprintType,Blueprintable)
struct FGameSessionData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString GameSessionId;
	UPROPERTY(BlueprintReadWrite)
	FString GameSessionName;
	UPROPERTY(BlueprintReadWrite)
	FString FleetId;
	UPROPERTY(BlueprintReadWrite)
	FDateTime CreationTime;
	UPROPERTY(BlueprintReadWrite)
	FDateTime TerminationTime;
	UPROPERTY(BlueprintReadWrite)
	int32 CurrentPlayerCount;
	UPROPERTY(BlueprintReadWrite)
	int32 MaxPlayerCount;
	UPROPERTY(BlueprintReadWrite)
	FString Status;
	UPROPERTY(BlueprintReadWrite)
	FString StatusReason;
	UPROPERTY(BlueprintReadWrite)
	FString IpAddress;
	UPROPERTY(BlueprintReadWrite)
	FString DnsName;
	UPROPERTY(BlueprintReadWrite)
	int32 Port;
	UPROPERTY(BlueprintReadWrite)
	FString CreatorId;
	UPROPERTY(BlueprintReadWrite)
	FString Location;
};

USTRUCT(BlueprintType)
struct FPlayerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString PlayerId;
	UPROPERTY(BlueprintReadOnly)
	FString PlayerSessionId;
	UPROPERTY(BlueprintReadOnly)
	FString Team;
};

USTRUCT(BlueprintType)
struct FGameSessionConnectionInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString GameSessionArn;
	UPROPERTY(BlueprintReadOnly)
	FString IPAddress;
	UPROPERTY(BlueprintReadOnly)
	FString DnsName;
	UPROPERTY(BlueprintReadOnly)
	int32 Port;
	UPROPERTY(BlueprintReadOnly)
	int32 EstimatedWaitTime;
};

USTRUCT(Blueprintable,BlueprintType)
struct FMatchmakingTicket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString TicketId;

	UPROPERTY(BlueprintReadOnly)
	FString ConfigurationName;
	
	UPROPERTY(BlueprintReadOnly)
	FString ConfigurationArn;
	
	UPROPERTY(BlueprintReadOnly)
	FString Status;
	
	UPROPERTY(BlueprintReadOnly)
	FString StatusReason;
	
	UPROPERTY(BlueprintReadOnly)
	FString StatusMessage;
	
	UPROPERTY(BlueprintReadOnly)
	FDateTime StartTime;
	
	UPROPERTY(BlueprintReadOnly)
	FDateTime EndTime;
	
	UPROPERTY(BlueprintReadOnly)	
	TArray<FPlayerData> Players;
	
	UPROPERTY(BlueprintReadOnly)	
	FGameSessionConnectionInfo ConnectionInfo;
};

USTRUCT(BlueprintType,Blueprintable)
struct FPlayerSessionData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite)
	FString PlayerSessionId;
	UPROPERTY(BlueprintReadWrite)
	FString PlayerId;
	UPROPERTY(BlueprintReadWrite)
	FString GameSessionId;
	UPROPERTY(BlueprintReadWrite)
	FString FleetId;
	UPROPERTY(BlueprintReadWrite)
	FString FleetArn;
	UPROPERTY(BlueprintReadWrite)
	int32 CreationTime;
	UPROPERTY(BlueprintReadWrite)
	int32 TerminationTime;
	UPROPERTY(BlueprintReadWrite)
	FString Status;
	UPROPERTY(BlueprintReadWrite)
	FString IpAddress;
	UPROPERTY(BlueprintReadWrite)
	FString DnsName;
	UPROPERTY(BlueprintReadWrite)
	int32 Port;
	UPROPERTY(BlueprintReadWrite)
	FString PlayerData;
};

USTRUCT(BlueprintType)
struct FMatchmakingTicketSubscriptionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString Status;

	UPROPERTY(BlueprintReadOnly)
	bool AcceptanceRequired = false;

	UPROPERTY(BlueprintReadOnly)
	FString GameSessionArn;

	UPROPERTY(BlueprintReadOnly)
	FString MatchId;

	UPROPERTY(BlueprintReadOnly)
	FString IpAddress;

	UPROPERTY(BlueprintReadOnly)
	int32 Port;

	UPROPERTY(BlueprintReadOnly)
	FString TicketId;

	UPROPERTY(BlueprintReadOnly)
	FString StartTime;

	UPROPERTY(BlueprintReadOnly)
	TArray<FPlayerData> Player;
};


USTRUCT(BlueprintType,Blueprintable)
struct FDescribeMatchmakingTicket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TArray<FMatchmakingTicket> Tickets;
};



DECLARE_DYNAMIC_DELEGATE_ThreeParams(FSearchComplete,const TArray<FGameSessionData>&, GameSessions,bool ,withError,EGameLiftExceptionsBP, exception );
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FSingleGameSessionResult,FGameSessionData, GameSession,bool ,withError,EGameLiftExceptionsBP, exception );
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FSingleSessionPlacementResult,FSessionPlacementData, SessionPlacement,bool ,withError,EGameLiftExceptionsBP, exception );
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FPlayerSessionResult,FPlayerSessionData, PlayerSession,bool ,withError,EGameLiftExceptionsBP, exception );
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FMatchMakingTiketResult,FMatchmakingTicket, MatchmakingTicket,bool ,withError,EGameLiftExceptionsBP, exception );
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FDescribeMatchmakingResult,const TArray<FMatchmakingTicketSubscriptionResult>&, MatchmakingTicket,bool ,withError,EGameLiftExceptionsBP, exception );
UCLASS()
class GAMELIFTBACKENDSERVICE_API UGLBSServiceConnector : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void GetSessions(FSearchComplete OnReady);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void CreateGameSession(FSingleGameSessionResult OnCreated,FString CreatorId,FString GameSessionName);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void QueueGameSession(FSingleSessionPlacementResult OnCreated,FString GameSessionName,FString PlacementId);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void CloseGameSession(FSingleGameSessionResult OnCreated,FString GameSessionId);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void GetPlayerSession(FPlayerSessionResult OnCreated, FString GameSessionId, FString PlayerId);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void StopMatchmaking(FString TicketId);
	
	UFUNCTION(BLueprintCallable, Category = "GLBS")
	static void StartMatchmaking(FMatchMakingTiketResult OnTicketCreated, FString PlayerId, FString GameMode,int32 skillLevel);

	UFUNCTION(BlueprintCallable, Category = "GLBS")
	static void CheckMatchmakingTicket(FDescribeMatchmakingResult OnTicketDescribed, TArray<FString> TicketIds);
private:
	TWeakObjectPtr<UWorld> WorldPtr;

	static FGameSessionData CreateGameSessionFromJson(const TSharedPtr<FJsonValue>& GameSessionJson);
	static FPlayerSessionData CreatePlayerSessionFromJson(const TSharedPtr<FJsonValue>& PlayerSessionJson);
	static FSessionPlacementData CreateSessionPlacementDataFromJson(const TSharedPtr<FJsonValue>& PlacementDataJson);
	static FDescribeMatchmakingTicket CreateMatchmakingTicketsDataFromJson(const TSharedPtr<FJsonObject>& MatchmakingTicketDataJson);
	static FMatchmakingTicket CreateMatchmakingTicketDataFromJson(const TSharedPtr<FJsonValue>& MatchmakingTicketDataJson);

	
	static TSharedRef<IHttpRequest, ESPMode::ThreadSafe> GetPostRequest(const FString& Endpoint, const TSharedPtr<FJsonObject>& JsonData);
	static TSharedRef<IHttpRequest, ESPMode::ThreadSafe> GetGetRequest(const FString& Endpoint);
};
