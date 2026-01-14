// Fill out your copyright notice in the Description page of Project Settings.


#include "GLBSServiceConnector.h"

#include "GameLiftExeptions.h"



TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UGLBSServiceConnector::GetPostRequest(const FString& Endpoint, const TSharedPtr<FJsonObject>& JsonData)
{
	TFunction<void(const FJsonObject& Result, const FString& Error)> Done;
	FHttpModule& Module= FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module.CreateRequest();
	Request->SetURL(Endpoint);
    Request->SetVerb("POST");
    Request->SetHeader("Content-Type", "application/json");

	// Convert to string
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonData.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);
	return Request;
}
TSharedRef<IHttpRequest, ESPMode::ThreadSafe> UGLBSServiceConnector::GetGetRequest(const FString& Endpoint)
{
	TFunction<void(const FJsonObject& Result, const FString& Error)> Done;
	FHttpModule& Module= FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module.CreateRequest();
	
	Request->SetURL(Endpoint);
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", "application/json");
	return Request;
}


void UGLBSServiceConnector::GetSessions(FSearchComplete OnReady)
{	

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetGetRequest("<your-endpoint>");


	Request->OnProcessRequestComplete().BindLambda([OnReady](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			TArray<FGameSessionData> GameSessions;
			FGameSessionData data;
			GameSessions.Add(data);
			OnReady.Execute(GameSessions,false,EGameLiftExceptionsBP::None);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSessions"))))
		{
			TArray<FGameSessionData> GameSessionsStruct;
			FGameSessionData data;
			TArray<TSharedPtr<FJsonValue>> GameSessionsJson = Json->GetArrayField(FString(TEXT("GameSessions")));
			for (TSharedPtr<FJsonValue> GameSession : GameSessionsJson)
			{
				data = CreateGameSessionFromJson(GameSession);
				GameSessionsStruct.Add(data);
			}			
			OnReady.Execute(GameSessionsStruct,false,EGameLiftExceptionsBP::None);
			return;
		}
		TArray<FGameSessionData> GameSessions;
		FGameSessionData data;
		GameSessions.Add(data);
		OnReady.Execute(GameSessions,false,EGameLiftExceptionsBP::None);

	});

	Request->ProcessRequest();
}

void UGLBSServiceConnector::CreateGameSession(FSingleGameSessionResult OnCreated,FString CreatorId,FString GameSessionName)
{
	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	JsonData->SetStringField(TEXT("CreatorId"),CreatorId);
	JsonData->SetStringField(TEXT("SessionName"),GameSessionName);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetPostRequest("<your-endpoint>",JsonData);
	
	
	Request->OnProcessRequestComplete().BindLambda([OnCreated](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		EGameLiftExceptions c = static_cast<EGameLiftExceptions>(Response->GetResponseCode());
		if (c == FLEET_CAPACITY_EXCEPTION)
		{	FGameSessionData data;
			OnCreated.Execute(data,true,EGameLiftExceptionsBP::Fleet_capacity_Exception);
			return;
		}
		
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			FGameSessionData data;
			OnCreated.Execute(data,true,EGameLiftExceptionsBP::Exception);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSession"))))
		{
			FGameSessionData data;
			TSharedPtr<FJsonValue> GameSessionsJson = Json->GetField(FString(TEXT("GameSession")),EJson::Object);
			data = CreateGameSessionFromJson(GameSessionsJson);
			OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
			return;
		}
		FGameSessionData data;
		OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
	});
	Request->ProcessRequest();
}

void UGLBSServiceConnector::QueueGameSession(FSingleSessionPlacementResult OnCreated, FString GameSessionName,
	FString PlacementId)
{

	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	JsonData->SetStringField(TEXT("SessionName"),GameSessionName);
	JsonData->SetStringField(TEXT("PlacementId"),PlacementId);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetPostRequest("<your-endpoint>",JsonData);
	
	Request->OnProcessRequestComplete().BindLambda([OnCreated](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			FSessionPlacementData data;
			OnCreated.Execute(data,true,EGameLiftExceptionsBP::Exception);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSession"))))
		{
			FSessionPlacementData data;
			TSharedPtr<FJsonObject> GameSession = Json->GetObjectField(FString(TEXT("GameSession")));
			if (GameSession->HasField(FString(TEXT("GameSession"))))
			{
				TSharedPtr<FJsonValue> GameSessionsJson = GameSession->GetField(FString(TEXT("GameSessions")),EJson::Object);
				data = CreateSessionPlacementDataFromJson(GameSessionsJson);
			}
			OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
			return;
		}
		FSessionPlacementData data;
		OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
	});
	Request->ProcessRequest();
}

void UGLBSServiceConnector::CloseGameSession(FSingleGameSessionResult OnClosed,FString GameSessionId)
{
	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	JsonData->SetStringField(TEXT("GameSessionId"),GameSessionId);
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetPostRequest("<your-endpoint>",JsonData);
	
	Request->OnProcessRequestComplete().BindLambda([OnClosed](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			FGameSessionData data;
			OnClosed.Execute(data,true,EGameLiftExceptionsBP::Exception);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSession"))))
		{
			FGameSessionData data;
			TSharedPtr<FJsonObject> GameSession = Json->GetObjectField(FString(TEXT("GameSession")));
			if (GameSession->HasField(FString(TEXT("GameSession"))))
			{
				TSharedPtr<FJsonValue> GameSessionsJson = GameSession->GetField(FString(TEXT("GameSessions")),EJson::Object);
				data = CreateGameSessionFromJson(GameSessionsJson);
			}
			OnClosed.Execute(data,false,EGameLiftExceptionsBP::None);
			return;
		}
		FGameSessionData data;
		OnClosed.Execute(data,false,EGameLiftExceptionsBP::None);
	});
	Request->ProcessRequest();
}

void UGLBSServiceConnector::StopMatchmaking(const FString TicketId)
{
	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	JsonData->SetStringField(TEXT("TickeId"),TicketId);
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetPostRequest("<your-endpoint>",JsonData);
	
	Request->ProcessRequest();
}


void UGLBSServiceConnector::GetPlayerSession(FPlayerSessionResult OnCreated, FString GameSessionId, FString PlayerId)
{
	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	JsonData->SetStringField(TEXT("GameSessionId"),GameSessionId);
	JsonData->SetStringField(TEXT("PlayerID"),PlayerId);
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetPostRequest("<your-endpoint>",JsonData);

	Request->OnProcessRequestComplete().BindLambda([OnCreated](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
{
	const FString ResponseString = Response->GetContentAsString();
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
	TSharedPtr<FJsonObject> Json;
	if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
	{
		FPlayerSessionData data;
		OnCreated.Execute(data,true,EGameLiftExceptionsBP::Exception);
		return;
	}
	if (Json->HasField(FString(TEXT("PlayerSession"))))
	{
		FPlayerSessionData data;
		TSharedPtr<FJsonObject> PlayerSession = Json->GetObjectField(FString(TEXT("PlayerSession")));

		if (Json->HasField(FString(TEXT("PlayerSession"))))
		{			
			TSharedPtr<FJsonValue> GameSessionsJson = Json->GetField(FString(TEXT("PlayerSession")),EJson::Object);
			data = CreatePlayerSessionFromJson(GameSessionsJson);
		}
		OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
		return;
	}
	FPlayerSessionData data;
	OnCreated.Execute(data,false,EGameLiftExceptionsBP::None);
});
	Request->ProcessRequest();
}

void UGLBSServiceConnector::StartMatchmaking(FMatchMakingTiketResult OnTicketCreated, FString PlayerId,
	FString GameMode,int32 skillLevel)
{
	TSharedPtr<FJsonObject> PlayerData = MakeShared<FJsonObject>();
	
	TSharedPtr<FJsonObject> PlayerAttributes = MakeShared<FJsonObject>();
	
	const TSharedPtr<FJsonObject> Skill = MakeShared<FJsonObject>();
	Skill->SetNumberField(TEXT("N"), skillLevel);
	
	const TSharedPtr<FJsonObject> GameModeAttr = MakeShared<FJsonObject>();
	GameModeAttr->SetStringField(TEXT("S"), GameMode);
	
	PlayerAttributes->SetObjectField(TEXT("skill"),Skill);
	PlayerAttributes->SetObjectField(TEXT("gamemode"),GameModeAttr);
	const TSharedPtr<FJsonObject> Latency = MakeShared<FJsonObject>();
	Latency->SetNumberField(TEXT("eu-central-1"),12);
	
	PlayerData->SetStringField(TEXT("PlayerId"),PlayerId);
	PlayerData->SetObjectField(TEXT("PlayerAttributes"),PlayerAttributes);
	PlayerData->SetObjectField(TEXT("Latency"),Latency);
	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	
	TArray<TSharedPtr<FJsonValue>> Players;
	Players.Add(MakeShared<FJsonValueObject>(PlayerData));
	JsonData->SetArrayField(TEXT("PlayerData"),Players);
	JsonData->SetStringField(TEXT("Config"),"NewConfiguration"); /// here you should paste the name of the AWS MAtchmaking Configuration
	
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetPostRequest("<your-endpoint>",JsonData);
	Request->OnProcessRequestComplete().BindLambda([OnTicketCreated](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())/// if no valid JSON is returned
		{
			FMatchmakingTicket data;
			OnTicketCreated.Execute(data,true,EGameLiftExceptionsBP::Exception);
			return;
		}
		if (Json->HasField(FString(TEXT("MatchmakingTicket"))))
		{
			FMatchmakingTicket data;
			TSharedPtr<FJsonValue> GameSessionsJson = Json->GetField(FString(TEXT("MatchmakingTicket")),EJson::Object);
			data = CreateMatchmakingTicketDataFromJson(GameSessionsJson);
			OnTicketCreated.Execute(data,false,EGameLiftExceptionsBP::None);
			return;
		}
		FMatchmakingTicket data;
		OnTicketCreated.Execute(data,false,EGameLiftExceptionsBP::None);
	});
	Request->ProcessRequest();
}

void UGLBSServiceConnector::CheckMatchmakingTicket(FDescribeMatchmakingResult OnTicketDescribed,
	TArray<FString> TicketIds)
{
	TSharedPtr<FJsonObject> JsonData = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> Tickets;
	for (const FString& TicketId : TicketIds)
	{
		Tickets.Add(MakeShared<FJsonValueString>(TicketId));
	}
	
	JsonData->SetArrayField(TEXT("TicketIds"),Tickets);
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = GetPostRequest("<your-endpoint>",JsonData);
	Request->OnProcessRequestComplete().BindLambda([OnTicketDescribed](FHttpRequestPtr, FHttpResponsePtr Response,bool bOK)
	{
		const FString ResponseString = Response->GetContentAsString();
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseString);
		TSharedPtr<FJsonObject> Json;
		if (!FJsonSerializer::Deserialize(Reader,Json) || !Json.IsValid())
		{
			TArray<FMatchmakingTicketSubscriptionResult> data;
			OnTicketDescribed.Execute(data,true,EGameLiftExceptionsBP::Exception);
			return;
		}
		if (Json->HasField(FString(TEXT("TicketList"))))
		{
			TArray<FMatchmakingTicketSubscriptionResult> data;
			//TSharedPtr<FJsonValue> GameSessionsJson = Json->GetField(FString(TEXT("TicketList")),EJson::Array);
			TArray<TSharedPtr<FJsonValue>> Tickets = Json->GetArrayField(TEXT("TicketList"));
			for (const TSharedPtr<FJsonValue>& TicketId : Tickets)
			{
				FMatchmakingTicketSubscriptionResult ticketdata;
				const TSharedPtr<FJsonObject> ticketObj = TicketId->AsObject();
				ticketdata.Status = ticketObj->GetStringField(TEXT("Status"));

				TArray<TSharedPtr<FJsonValue>> players = ticketObj->GetArrayField(TEXT("players"));
				for (const TSharedPtr<FJsonValue>& player : players)
				{
					FPlayerData playerData;
					TSharedPtr<FJsonObject> playerObj = player->AsObject();
					playerData.PlayerId = playerObj->GetStringField(TEXT("playerId"));
					if (playerObj->HasField(TEXT("team")))
					{
						playerData.Team = playerObj->GetStringField(TEXT("team"));
					}
					if (playerObj->HasField(TEXT("playerSessionId")))
					{
						playerData.PlayerSessionId = playerObj->GetStringField(TEXT("playerSessionId"));
					}
					ticketdata.Player.Add(playerData);
				}
				if (ticketObj->HasField(TEXT("acceptanceRequired")))
				{
					ticketdata.AcceptanceRequired = ticketObj->GetBoolField(TEXT("acceptanceRequired"));
				}
				if (ticketObj->HasField(TEXT("gameSessionArn")))
				{
					ticketdata.GameSessionArn = ticketObj->GetStringField(TEXT("gameSessionArn"));
				}
				if (ticketObj->HasField(TEXT("ipAddress")))
				{
					ticketdata.IpAddress = ticketObj->GetStringField(TEXT("ipAddress"));
				}
				if (ticketObj->HasField(TEXT("port")))
				{
					ticketdata.Port = ticketObj->GetIntegerField(TEXT("port"));
				}
				if (ticketObj->HasField(TEXT("matchId")))
				{
					ticketdata.MatchId = ticketObj->GetStringField(TEXT("matchId"));
				}
				data.Add(ticketdata);
			}
			OnTicketDescribed.Execute(data,false,EGameLiftExceptionsBP::None);
			return;
		}
		TArray<FMatchmakingTicketSubscriptionResult> data;
		OnTicketDescribed.Execute(data,false,EGameLiftExceptionsBP::None);
	});
	Request->ProcessRequest();
}

FGameSessionData UGLBSServiceConnector::CreateGameSessionFromJson(const TSharedPtr<FJsonValue>& GameSessionJson)
{
	FGameSessionData data;
	TSharedPtr<FJsonObject> obj = GameSessionJson->AsObject();
	if (obj->HasField(TEXT("GameSessionId")))
	{
		data.GameSessionId =obj->GetStringField(TEXT("GameSessionId"));
	}
	if (obj->HasField(TEXT("Name")))
	{
		data.GameSessionName =obj->GetStringField(TEXT("Name"));
	}
	if (obj->HasField(TEXT("FleetId")))
	{
		data.FleetId =obj->GetStringField(TEXT("FleetId"));
	}
	if (obj->HasField(TEXT("CreationTime")))
	{
		//FString CreationTime= obj->GetStringField(TEXT("CreationTime"));
		FDateTime Time;
		FDateTime::ParseIso8601(*obj->GetStringField(TEXT("CreationTime")),Time);
		data.CreationTime =Time;
	}
	if (obj->HasField(TEXT("TerminationTime")))
	{
		//FString CreationTime= obj->GetStringField(TEXT("TerminationTime"));
		FDateTime Time;
		FDateTime::ParseIso8601(*obj->GetStringField(TEXT("TerminationTime")),Time);
		data.TerminationTime =Time;
	}
	if (obj->HasField(TEXT("CurrentPlayerSessionCount")))
	{
		data.CurrentPlayerCount =obj->GetIntegerField(TEXT("CurrentPlayerSessionCount"));
	}
	if (obj->HasField(TEXT("MaximumPlayerSessionCount")))
	{
		data.MaxPlayerCount =obj->GetIntegerField(TEXT("MaximumPlayerSessionCount"));
	}
	if (obj->HasField(TEXT("Status")))
	{
		data.Status =obj->GetStringField(TEXT("Status"));
	}
	if (obj->HasField(TEXT("StatusReason")))
	{
		data.StatusReason =obj->GetStringField(TEXT("StatusReason"));
	}
	if (obj->HasField(TEXT("IpAddress")))
	{
		data.IpAddress =obj->GetStringField(TEXT("IpAddress"));
	}
	if (obj->HasField(TEXT("DnsName")))
	{
		data.DnsName =obj->GetStringField(TEXT("DnsName"));
	}
	if (obj->HasField(TEXT("Port")))
	{
		data.Port =obj->GetIntegerField(TEXT("Port"));
	}
	if (obj->HasField(TEXT("CreatorId")))
	{
		data.CreatorId =obj->GetStringField(TEXT("CreatorId"));
	}
	if (obj->HasField(TEXT("Location")))
	{
		data.Location =obj->GetStringField(TEXT("Location"));
	}
	return data;
}
FSessionPlacementData UGLBSServiceConnector::CreateSessionPlacementDataFromJson(const TSharedPtr<FJsonValue>& PlacementDataJson)
{
	FSessionPlacementData data;

	TSharedPtr<FJsonObject> obj = PlacementDataJson->AsObject();
	if (obj->HasField(TEXT("GameSessionName")))
	{
		data.GameSessionName =obj->GetStringField(TEXT("GameSessionName"));
	}
	if (obj->HasField(TEXT("GameSessionQueueName")))
	{
		data.GameSessionQueueName =obj->GetStringField(TEXT("GameSessionQueueName"));
	}
	if (obj->HasField(TEXT("MaximalPlayerSessionCount")))
	{
		data.MaximumPlayerSessionCount =obj->GetIntegerField(TEXT("MaximalPlayerSessionCount"));
	}
	if (obj->HasField(TEXT("PlacementId")))
	{
		data.PlacementId =obj->GetStringField(TEXT("PlacementId"));
	}
	if (obj->HasField(TEXT("Status")))
	{
		data.Status =obj->GetStringField(TEXT("Status"));
	}
	if (obj->HasField(TEXT("StartTime")))
	{
		FDateTime Time;
		FDateTime::ParseIso8601(*obj->GetStringField(TEXT("StartTime")),Time);
		data.StartTime =Time;
	}
	return data;
}

FMatchmakingTicket UGLBSServiceConnector::CreateMatchmakingTicketDataFromJson(
	const TSharedPtr<FJsonValue>& MatchmakingTicketDataJson)
{
	FMatchmakingTicket data;
	TSharedPtr<FJsonObject> obj = MatchmakingTicketDataJson->AsObject();

	if (obj->HasField(TEXT("TicketId")))
	{
		data.TicketId =obj->GetStringField(TEXT("TicketId"));
	}
	if (obj->HasField(TEXT("ConfigurationName")))
	{
		data.ConfigurationName =obj->GetStringField(TEXT("ConfigurationName"));
	}
	if (obj->HasField(TEXT("ConfigurationArn")))
	{
		data.ConfigurationArn =obj->GetStringField(TEXT("ConfigurationArn"));
	}
	if (obj->HasField(TEXT("Status")))
	{
		data.Status =obj->GetStringField(TEXT("Status"));
	}
	if (obj->HasField(TEXT("StatusReason")))
	{
		data.StatusReason =obj->GetStringField(TEXT("StatusReason"));
	}
	if (obj->HasField(TEXT("StatusMessage")))
	{
		data.StatusMessage =obj->GetStringField(TEXT("StatusMessage"));
	}
	if (obj->HasField(TEXT("StartTime")))
	{
		FDateTime Time;
		FDateTime::ParseIso8601(*obj->GetStringField(TEXT("StartTime")),Time);
		data.StartTime =Time;
	}
	if (obj->HasField(TEXT("EndTime")))
	{
		FDateTime Time;
		FDateTime::ParseIso8601(*obj->GetStringField(TEXT("EndTime")),Time);
		data.EndTime =Time;
	}

	if (obj->HasField(TEXT("GameSessionConnectionInfo")))
	{
		FGameSessionConnectionInfo connectionInfo;
		TSharedPtr<FJsonObject> connectionInfoJson = obj->GetObjectField(TEXT("GameSessionConnectionInfo"));
		if (connectionInfoJson->HasField(TEXT("GameSessionArn")))
		{
			connectionInfo.GameSessionArn =connectionInfoJson->GetStringField(TEXT("GameSessionArn"));
		}
		if (connectionInfoJson->HasField(TEXT("IpAddress")))
		{
			connectionInfo.IPAddress =connectionInfoJson->GetStringField(TEXT("IpAddress"));
		}
		if (connectionInfoJson->HasField(TEXT("DnsName")))
		{
			connectionInfo.DnsName =connectionInfoJson->GetStringField(TEXT("DnsName"));
		}
		if (connectionInfoJson->HasField(TEXT("Port")))
		{
			connectionInfo.Port =connectionInfoJson->GetIntegerField(TEXT("Port"));
		}
		data.ConnectionInfo = connectionInfo;
	}
	return data;
}

FDescribeMatchmakingTicket UGLBSServiceConnector::CreateMatchmakingTicketsDataFromJson(
	const TSharedPtr<FJsonObject>& MatchmakingTicketDataJson)
{

	FDescribeMatchmakingTicket data;
	TSharedPtr<FJsonObject> obj = MatchmakingTicketDataJson;

	TArray<TSharedPtr<FJsonValue>> Tickets = obj->GetArrayField(TEXT("TicketList"));
	for (TSharedPtr<FJsonValue> Ticket : Tickets)
	{
		data.Tickets.Add(CreateMatchmakingTicketDataFromJson(Ticket));
	}
	
	return data;
}



FPlayerSessionData UGLBSServiceConnector::CreatePlayerSessionFromJson(const TSharedPtr<FJsonValue>& PlayerSessionJson)
{
	FPlayerSessionData data;
	TSharedPtr<FJsonObject> obj = PlayerSessionJson->AsObject();
	if (obj->HasField(TEXT("PlayerSessionId")))
	{
		data.PlayerSessionId =obj->GetStringField(TEXT("PlayerSessionId"));
	}
	if (obj->HasField(TEXT("PlayerId")))
	{
		data.PlayerId =obj->GetStringField(TEXT("PlayerId"));
	}
	if (obj->HasField(TEXT("GameSessionId")))
	{
		data.GameSessionId =obj->GetStringField(TEXT("GameSessionId"));
	}
	if (obj->HasField(TEXT("FleetId")))
	{
		data.FleetId =obj->GetStringField(TEXT("FleetId"));
	}
	if (obj->HasField(TEXT("FleetArn")))
	{
		data.FleetArn =obj->GetStringField(TEXT("FleetArn"));
	}
	if (obj->HasField(TEXT("CreationTime")))
	{
		data.CreationTime =obj->GetIntegerField(TEXT("CreationTime"));
	}
	if (obj->HasField(TEXT("TerminationTime")))
	{
		data.TerminationTime =obj->GetIntegerField(TEXT("TerminationTime"));
	}
	if (obj->HasField(TEXT("Status")))
	{
		data.Status =obj->GetStringField(TEXT("Status"));
	}
	if (obj->HasField(TEXT("IpAddress")))
	{
		data.IpAddress =obj->GetStringField(TEXT("IpAddress"));
	}
	if (obj->HasField(TEXT("DnsName")))
	{
		data.DnsName =obj->GetStringField(TEXT("DnsName"));
	}
	if (obj->HasField(TEXT("Port")))
	{
		data.Port =obj->GetIntegerField(TEXT("Port"));
	}
	if (obj->HasField(TEXT("PlayerData")))
	{
		data.PlayerData =obj->GetStringField(TEXT("PlayerData"));
	}
	
	return data;
}


