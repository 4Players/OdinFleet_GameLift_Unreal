# OdinFleet AWS GameLift Integration

This guide covers the full integration of a dedicated Unreal Engine game server with **Amazon GameLift** via **OdinFleet**. It walks through building the server, configuring the GameLift SDK, setting up the GameLift Agent, containerising the server with Docker, deploying through OdinFleet, and connecting a game client through a backend service.

## Table of Contents

- [Requirements](#requirements)
- [Dedicated Game Server](#dedicated-game-server)
- [GameLift Server SDK](#gamelift-server-sdk)
- [GameLift Server Agent](#gamelift-server-agent)
- [Docker](#docker)
- [OdinFleet](#odinfleet)
- [Backend Service](#backend-service)
- [Unreal Game Client](#unreal-game-client)
- [Communication Overview](#communication-overview)

---

## Requirements

- Dedicated Game Server (Unreal Engine)
- GameLift Server SDK
- GameLift Server Agent
- Docker

---

## Dedicated Game Server

If you do not already have one, build an Unreal Engine Dedicated Server. A full guide is available in the [Unreal Engine documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/setting-up-dedicated-servers?application_version=4.27). The linked example targets UE 4.27, but the steps are identical for UE 5.x.

---

## GameLift Server SDK

To connect the game server with Amazon GameLift, the server requires the GameLift Server SDK. Available versions are listed in the [AWS documentation](https://docs.aws.amazon.com/gameliftservers/latest/developerguide/reference-serversdk.html).

This project uses the [C++ SDK for Unreal](https://github.com/amazon-gamelift/amazon-gamelift-plugin-unreal). Download the repository and build the SDK.

**Linux / macOS:**

```bash
chmod +x setup.sh
sh setup.sh
```

**Windows:**

```powershell
powershell -file setup.ps1
```

### Installing the Plugin

After the build completes, copy the SDK into your Unreal project's `Plugins` directory. Two options are available:

| Folder | Contents |
|---|---|
| `GameLiftServerSDK` | Server SDK only |
| `GameLiftPlugin` | Server SDK **plus** additional Editor UI components |

Choose whichever best fits your needs.

### Adding the Module Dependency

Add the plugin to `PublicDependencyModules` in your project's `<ProjectName>.Build.cs`:

```cpp
    if (Target.Type == TargetType.Server)
    {
        PublicDependencyModuleNames.Add("GameLiftServerSDK");
    }else{
        PublicDefinitions.Add("WITH_GAMELIFT=0");
    }
    bEnableExceptions =  true;
```

### Integrating the SDK in the Game Mode

#### Header (`YourGameMode.h`)

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "YourGameMode.generated.h"

struct FProcessParameters;

DECLARE_LOG_CATEGORY_EXTERN(GameServerLog, Log, All);

UCLASS(minimalapi)
class AYourGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AYourGameMode();

protected:
    virtual void BeginPlay() override;

private:
    void InitiateGameLift();

private:
    TSharedPtr<FProcessParameters> ProcessParameters;
}; 
```

#### Source - Includes and Constructor (`YourGameMode.cpp`)

```cpp
#include "YourGameMode.h"

#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

#if WITH_GAMELIFT
#include "GameLiftServerSDK.h"
#include "GameLiftServerSDKModels.h"
#endif

#include "GenericPlatform/GenericPlatformOutputDevices.h"
DEFINE_LOG_CATEGORY(GameServerLog);

AYourGameMode::AYourGameMode() : ProcessParameters(nullptr)
{
  ...//Your contructor code
}
```

#### Source - `InitiateGameLift` Implementation

```cpp
void AYourGameMode::InitiateGameLift()
{
    #if WITH_GAMELIFT
    UE_LOG(GameServerLog, Log, TEXT("Calling InitGameLift..."));
    FGameLiftServerSDKModule* GameLiftSdkModule = &FModuleManager::LoadModuleChecked<FGameLiftServerSDKModule>(FName("GameLiftServerSDK"));

    UE_LOG(GameServerLog, Log, TEXT("Initializing the GameLift Server..."));
    //InitSDK will establish a local connection with GameLift's agent to enable further communication.
    FGameLiftGenericOutcome InitSdkOutcome = GameLiftSdkModule->InitSDK();

        if (InitSdkOutcome.IsSuccess())
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_GREEN);
        UE_LOG(GameServerLog, Log, TEXT("GameLift InitSDK succeeded!"));
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
    }
    else
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_RED);
        UE_LOG(GameServerLog, Log, TEXT("ERROR: InitSDK failed : ("));
        FGameLiftError GameLiftError = InitSdkOutcome.GetError();
        UE_LOG(GameServerLog, Log, TEXT("ERROR: %s"), *GameLiftError.m_errorMessage);
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
        return;
    }

        ProcessParameters = MakeShared<FProcessParameters>();

    //When a game session is created, Amazon GameLift Servers sends an activation request to the game server and passes along the game session object containing game properties and other settings.
    //Here is where a game server should take action based on the game session object.
    //Once the game server is ready to receive incoming player connections, it should invoke GameLiftServerAPI.ActivateGameSession()
    ProcessParameters->OnStartGameSession.BindLambda([=](Aws::GameLift::Server::Model::GameSession InGameSession)
        {
            FString GameSessionId = FString(InGameSession.GetGameSessionId());
            UE_LOG(GameServerLog, Log, TEXT("GameSession Initializing: %s"), *GameSessionId);
            GameLiftSdkModule->ActivateGameSession();
        });

    //OnProcessTerminate callback. Amazon GameLift Servers will invoke this callback before shutting down an instance hosting this game server.
    //It gives this game server a chance to save its state, communicate with services, etc., before being shut down.
    //In this case, we simply tell Amazon GameLift Servers we are indeed going to shutdown.
    ProcessParameters->OnTerminate.BindLambda([=]()
	{
		UE_LOG(GameServerLog, Log, TEXT("Game Server Process is terminating"));
		FGameLiftGenericOutcome processEndingOutcome = GameLiftServerSdkModule->ProcessEnding();

		FGameLiftGenericOutcome destroyOutcome = GameLiftServerSdkModule->Destroy();
		if (processEndingOutcome.IsSuccess() && destroyOutcome.IsSuccess())
		{
			UE_LOG(GameServerLog, Log, TEXT("Server process ending successfully"));
			FGenericPlatformMisc::RequestExit(false); //Important, otherwise it could remain an process open, that blocks the used port
		}else{
			if (!processEndingOutcome.IsSuccess()) {
				const FGameLiftError& error = processEndingOutcome.GetError();
				UE_LOG(GameServerLog, Error, TEXT("ProcessEnding() failed. Error: %s"),
				error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
			}
			if (!destroyOutcome.IsSuccess()) {
				const FGameLiftError& error = destroyOutcome.GetError();
				UE_LOG(GameServerLog, Error, TEXT("Destroy() failed. Error: %s"),
				error.m_errorMessage.IsEmpty() ? TEXT("Unknown error") : *error.m_errorMessage);
			}
		}
	});
        //This is the HealthCheck callback.
    //Amazon GameLift Servers will invoke this callback every 60 seconds or so.
    //Here, a game server might want to check the health of dependencies and such.
    //Simply return true if healthy, false otherwise.
    //The game server has 60 seconds to respond with its health status. Amazon GameLift Servers will default to 'false' if the game server doesn't respond in time.
    //In this case, we're always healthy!
    ProcessParameters->OnHealthCheck.BindLambda([]()
        {
            UE_LOG(GameServerLog, Log, TEXT("Performing Health Check"));
            return true;
        });

    //GameServer.exe -port=7777 LOG=server.mylog
    ProcessParameters->port = FURL::UrlConfig.DefaultPort;
    TArray<FString> CommandLineTokens;
    TArray<FString> CommandLineSwitches;

    FCommandLine::Parse(FCommandLine::Get(), CommandLineTokens, CommandLineSwitches);

    for (FString SwitchStr : CommandLineSwitches)
    {
        FString Key;
        FString Value;

        if (SwitchStr.Split("=", &Key, &Value))
        {
            if (Key.Equals("port"))
            {
                ProcessParameters->port = FCString::Atoi(*Value);
            }
        }
    }
    //Here, the game server tells Amazon GameLift Servers where to find game session log files.
    //At the end of a game session, Amazon GameLift Servers uploads everything in the specified 
    //location and stores it in the cloud for access later.
    TArray<FString> Logfiles;
    Logfiles.Add(TEXT("GameLiftUnrealApp/Saved/Logs/server.log"));
    ProcessParameters->logParameters = Logfiles;

    //The game server calls ProcessReady() to tell Amazon GameLift Servers it's ready to host game sessions.
    UE_LOG(GameServerLog, Log, TEXT("Calling Process Ready..."));
    FGameLiftGenericOutcome ProcessReadyOutcome = GameLiftSdkModule->ProcessReady(*ProcessParameters);

    if (ProcessReadyOutcome.IsSuccess())
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_GREEN);
        UE_LOG(GameServerLog, Log, TEXT("Process Ready!"));
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
    }
    else
    {
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_RED);
        UE_LOG(GameServerLog, Log, TEXT("ERROR: Process Ready Failed!"));
        FGameLiftError ProcessReadyError = ProcessReadyOutcome.GetError();
        UE_LOG(GameServerLog, Log, TEXT("ERROR: %s"), *ProcessReadyError.m_errorMessage);
        UE_LOG(GameServerLog, SetColor, TEXT("%s"), COLOR_NONE);
    }

    UE_LOG(GameServerLog, Log, TEXT("InitGameLift completed!"));
    #endif
}
```

#### Source - `BeginPlay`

```cpp
void AYourGameMode::BeginPlay()
{
	Super::BeginPlay();

#if WITH_GAMELIFT
	InitGameLift();
	#endif
}
```

### Anywhere Fleet Parameters

You do not need to initialise or set any parameters used for Anywhere Fleets manually; the GameLift Server Agent handles this automatically. If you are interested in configuring these parameters yourself, refer to the `README.md` included in the plugin folder.

### Minimum Server Requirements

At a minimum, the game server must:

1. Call `InitSDK()`.
2. Implement the game session callbacks (`OnStartGameSession`, `OnTerminate`, `OnHealthCheck`).
3. Set the listening port.
4. Call `ProcessReady()`.

---

## GameLift Server Agent

The Server Agent handles all communication with Amazon GameLift:

- Authenticates with your AWS account via an access key.
- Registers a compute device.
- Reads and periodically refreshes the authentication token for the compute device.
- Starts the game server with the configured parameters.
- Manages the heartbeat.

### AWS Account and User Permissions

1. Create an [AWS account](https://aws.amazon.com/) (if you do not already have one).
2. Navigate to Identity and Access Management (IAM) and create a new IAM user.
3. Select the new user and create an access key under Security credentials > Create access key. Store the key securely.

The IAM user requires permissions for the following actions:

- `gamelift:RegisterCompute`
- `gamelift:GetComputeAuthToken`
- `gamelift:DeregisterCompute`

The recommended approach is to create a dedicated IAM policy and attach it to the user.

Navigate to **IAM > Policies > Create policy**. You can configure permissions via the visual editor or switch to JSON and paste the following:

```json
{
	"Version": "2012-10-17",
	"Statement": [
		{
			"Sid": "AllowRegisterComputeOnFleet",
			"Effect": "Allow",
			"Action": "gamelift:RegisterCompute",
			"Resource": "arn:aws:gamelift:<your-region>:<your-account-id>:fleet/<your-fleet-id>"
		},
		{
			"Sid": "DeregisterCompute",
			"Effect": "Allow",
			"Action": "gamelift:DeregisterCompute",
			"Resource": "arn:aws:gamelift:<your-region>:<your-account-id>:fleet/<your-fleet-id>"
		},
		{
			"Sid": "GetAuthTokenForFleet",
			"Effect": "Allow",
			"Action": "gamelift:GetComputeAuthToken",
			"Resource": "arn:aws:gamelift:<your-region>:<your-account-id>:fleet/<your-fleet-id>"
		}
	]
}
```

Click **Next > Save Changes**, then return to the user and attach the newly created policy.

### GameLift Location and Fleet

1. Navigate to [Amazon GameLift Servers](https://eu-central-1.console.aws.amazon.com/gameliftservers/dashboard) and create a custom location for your Anywhere Fleet.

   ![Custom Location](Location.jpg)

2. Create an `Anywhere Fleet`, assign it a name, and select your custom location.

   ![Anywhere Fleet](Fleet.jpg)

### Building the Agent

Download the agent from the [GitHub repository](https://github.com/amazon-gamelift/amazon-gamelift-agent).

**Prerequisites:**

- Java 17 or later
- Maven 3.2.5 or later

Verify the installed versions:

```bash
java -version
mvn -version
```

If either tool is missing or below the required version, download them here:

- [Java JDK 17](https://www.oracle.com/java/technologies/javase/jdk17-archive-downloads.html)
- [Apache Maven](https://maven.apache.org/download.cgi)

Once both prerequisites are satisfied, open a terminal in the agent's root directory (where `pom.xml` is located) and build:

```bash
mvn clean compile assembly:single
```

On success, the output JAR is located at:

```
./target/GameLiftAgent-1.0.jar
```

This JAR file will serve as the entry point for the Docker image created in a later step.

### Runtime Configuration

The agent needs to know where the server executable is located. This information is provided via a `runtime-config.json` file:

```json
{
  "ServerProcesses": [
    {
      "LaunchPath": "/local/game/<server-executable>",
      "Parameters": "server parameter",
      "ConcurrentExecutions": 1
    }
  ],
  "MaxConcurrentGameSessionActivations": 1,
  "GameSessionActivationTimeoutSeconds": 300
}
```

On Windows, the `LaunchPath` must start with `C:/Game/`. On Linux, it must start with `/local/game/`.

For the full specification, see the [RuntimeConfiguration API reference](https://docs.aws.amazon.com/gameliftservers/latest/apireference/API_RuntimeConfiguration.html).

---

## Docker

OdinFleet uses Docker images to deploy the game server. Install [Docker Desktop](https://docs.docker.com/desktop/setup/install/windows-install/), create a Docker account, and enable Windows Subsystem for Linux (WSL):

```shell
wsl --install
```

### Building the Linux Server

The Docker image requires a Linux build of the game server. Install the Linux cross-compilation toolchain by following the [Unreal Engine Linux development guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/linux-development-requirements-for-unreal-engine?application_version=5.0) and downloading the correct version.

Verify the installation:

```shell
echo %LINUX_MULTIARCH_ROOT%
```

**Packaging via the Editor:**

Open the Unreal Editor and package the server:

![Editor Package](Package.jpg)

**Packaging via the command line:**

```bat
<path-to-source-built-engine>/Engine/Build/BatchFiles/RunUAT.bat BuildCookRun ^
 -project="<path-to-your-project>/<YourProject>.uproject" ^
 -noP4 -server -platform=Linux -clientconfig=Shipping -serverconfig=Shipping ^
 -cook -allmaps -build -stage -pak -archive ^
 -archivedirectory="<Your-package-folder>"
```

### Preparing the Docker Image

Create a working directory (e.g. `DockerImageData`) containing the following files:

```
DockerImageData/
|- <Your-package-folder>/
|- GameLiftAgent-1.0.jar
|- runtime-config.json
|- entrypoint.sh
|- Dockerfile
```

#### `Dockerfile`

The Dockerfile copies the required files into the image and sets the entry point - the script or executable invoked when the container starts.

```dockerfile
FROM ghcr.io/epicgames/unreal-engine:runtime

USER root

# install java to run the agent.jar this need root user permissions
RUN apt-get update && apt-get install -y --no-install-recommends openjdk-17-jre-headless \
 && rm -rf /var/lib/apt/lists/*
# switch to ue4 user. Some executions cant be made as root
USER ue4
# Put game under /local/game and make ue4 the owner
WORKDIR /local/game
COPY --chown=ue4:ue4 LinuxServer/ /local/game/

# copy Agent and entrypoint; also owned by ue4
COPY --chown=ue4:ue4 GameLiftAgent-1.0.jar /gamelift/agent.jar
COPY --chown=ue4:ue4 entrypoint.sh /entrypoint.sh

# create log folders and set permissions for user ue4
RUN chmod 0755 /entrypoint.sh \
 && mkdir -p /local/game/Saved/Logs /local/game/logs /local/gameliftagent \
 && chown -R ue4:ue4 /local /gamelift


ENTRYPOINT ["/entrypoint.sh"]
```

#### `entrypoint.sh`

The entry-point script reads environment variables and passes them to the agent:

```bash
#!/usr/bin/env bash
set -euo pipefail

# If COMPUTE_NAME isn't provided, generate one
if [[ -z "${COMPUTE_NAME:-}" || "${COMPUTE_NAME}" == "auto" ]]; then
  # /proc/sys/kernel/random/uuid exists by default and doesn't need uuidgen
  RAND_ID="$(cat /proc/sys/kernel/random/uuid)"
  # Optional: prefix to recognize the node in AWS console
  COMPUTE_NAME="odin-$(echo "$RAND_ID" | tr '[:upper:]' '[:lower:]')"
fi
: "${REGION:?Set REGION}"
export AWS_REGION="${AWS_REGION:-$REGION}"
: "${FLEET_ID:?Set FLEET_ID}"
: "${LOCATION:?Set LOCATION}"
: "${REGION:?Set REGION}"
: "${PUBLIC_IP:?Set PUBLIC_IP}"

exec java -jar /gamelift/agent.jar -c "${COMPUTE_NAME}" -f "${FLEET_ID}" -loc "${LOCATION}" -r "${REGION}" -ip-address "${PUBLIC_IP}"
```

The compute name is auto-generated. When the agent shuts down, the registered compute remains in the `TERMINATING` state for approximately 1-3 days before AWS resets it to `ACTIVE`. Attempting to register an existing compute name that is still in `TERMINATING` will cause registration to fail and prevent the agent from connecting.

### Runtime Configuration Delivery

You can either upload the configuration to AWS or embed it directly in the image.

**Option A - Upload via the [AWS CLI](https://docs.aws.amazon.com/cli/latest/userguide/getting-started-install.html):**

```bash
aws gamelift update-runtime-configuration \
  --fleet-id <your-fleet-id> \
  --runtime-configuration file://<path-to-config>/runtime-config.json \
  --region <your-region>
```

The agent loads the configuration automatically.

**Option B - Inline in the image:**

Add `-runtime-configuration` (or `-rc`) as a parameter to the `exec java -jar /gamelift/agent.jar` call and pass the configuration as inline JSON.

### Server Launch Script

The Unreal Linux build includes a `GameServer.sh` in its root directory. This script invokes the server executable located at `LinuxServer/<your-project-name>/Binaries/Linux`. Use this `.sh` script as the `LaunchPath` in your runtime configuration.

The game server must listen on the OdinFleet server's external port rather than the Unreal default port (7777). The following example shows how to forward the port via the launch script:

```bash
#!/bin/sh
UE_TRUE_SCRIPT_NAME=$(echo \"$0\" | xargs readlink -f)
UE_PROJECT_ROOT=$(dirname "$UE_TRUE_SCRIPT_NAME")
chmod +x "$UE_PROJECT_ROOT/<your-project-name>/Binaries/Linux/<your-project-executable>"

PORT_ARG=""
if [ -n "${EXTERNAL_PORT:-}" ]; then
  PORT_ARG="-port=${EXTERNAL_PORT}"
fi

"$UE_PROJECT_ROOT/<your-project-name>/Binaries/Linux/<your-project-executable>" <your-project-name> "$@" $PORT_ARG
```

**Warning:** This file is overwritten each time you repackage the project. Remember to reapply your changes after every build.

### Building the Docker Image

Open a terminal in the directory containing the Dockerfile and run:

```bash
docker build -t <your-image-name>:<your-image-tag> .
```

### Local Testing

You can test the image locally in Docker Desktop. Navigate to `Images`, locate your server image, and click `Run`. Configure the following environment variables and port mapping:

**Application variables:**

| Variable | Description |
|---|---|
| `FLEET_ID` | Your GameLift Anywhere Fleet ID |
| `LOCATION` | Your custom GameLift location |
| `REGION` | AWS region (e.g. `eu-central-1`) |
| `PUBLIC_IP` | IP address of the OdinFleet server |
| `EXTERNAL_PORT` | Port the game server should bind to |

**AWS credentials (used internally by the agent):**

| Variable | Description |
|---|---|
| `AWS_ACCESS_KEY_ID` | IAM user access key ID |
| `AWS_SECRET_ACCESS_KEY` | IAM user secret access key |

### Pushing to Docker Hub

Once the image works locally, tag and push it:

```bash
docker tag <your-image-name>:<image-tag> <docker-username>/<your-image-name>:<release-tag>
# <release-tag> can be a version number or a label such as "latest" or "release"

docker push <docker-username>/<your-image-name>:<release-tag>
```

---

## OdinFleet

The final step is to deploy the image on an OdinFleet server. Follow the guide for creating a Fleet App:

- [Minecraft server example](https://docs.4players.io/fleet/guides/getting-started/)
- [Unreal game server example](https://docs.4players.io/fleet/guides/unreal-server/)

### Server Configuration

1. Create a port in the **Port Settings** as described in the guides above:

   ![Create Port](Port.jpg)

2. Add a **Dynamic Variable** to the environment variables:

   ![Dynamic Variable](DynamicVar.jpg)

   This maps the OdinFleet server port to the `EXTERNAL_PORT` environment variable, which is consumed by the game server and forwarded to GameLift.

   The resulting port mapping looks like this:

   ```
   Server Port : 12345
   Image Port  : 12345
   Mapping     : 12345 > 12345
   ```

---

## Backend Service

To avoid storing AWS access keys on the client, the client itself should not communicate directly with AWS to manage game sessions. Instead, this communication is handled by a backend service - typically a REST API.

The client communicates with this API, which uses AWS credentials in a secure, server-side environment.

### Setting Up the AWS SDK

Install the [AWS GameLift client SDK](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/):

```bash
# npm
npm install @aws-sdk/client-gamelift

# yarn
yarn add @aws-sdk/client-gamelift

# pnpm
pnpm add @aws-sdk/client-gamelift
```

You can use any Node.js web framework of your choice. The examples below use Google Cloud Run Functions (Firebase). The specific framework initialisation is not covered here; the key point is that these functions behave like standard HTTPS endpoints.

### General Pattern

The AWS GameLift client SDK follows a consistent pattern:

1. Create an input object.
2. Instantiate the appropriate command.
3. Pass the input to the command.
4. Execute the command via the client.

### Initialisation

```js
const {onRequest} = require("firebase-functions/v2/https");
const {GameLiftClient, SearchGameSessionsCommand, CreateGameSessionCommand, TerminateGameSessionCommand} = require('@aws-sdk/client-gamelift');

const FleetID = "<your-fleet-id>";
const Location = "your-location";
const AWSRegion = "your-aws-region";//example: eu-central-1
const GCloudRegion = "your-gcloud-region";//example: europe-west3
// Create and deploy your first functions
// https://firebase.google.com/docs/functions/get-started

// exports.helloWorld = onRequest((request, response) => {
//   logger.info("Hello logs!", {structuredData: true});
//   response.send("Hello from Firebase!");
// });


//Create the GameliftClient
const gameLiftClient = new GameLiftClient({
    region:"eu-central-1",
    credentials:{
        accessKeyId:"",
        secretAccessKey:""
    }
});
```

**Important:** Store your AWS credentials in a credential file, environment variable, or a secure secrets manager - never hard-code them.

### Example: Search Game Sessions

See the [SearchGameSessionsCommand reference](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/Class/SearchGameSessionsCommand/).

```js
exports.<your-function-name> = onRequest({region:GCloudRegion},async(req,res)=>{

    const SearchInput = {
        FleetId:FleetID,
        Location:Location,
    };
    const command = new SearchGameSessionsCommand(SearchInput);
    await executeCommand(res,command);
    return;
});

async function executeCommand(res,command){
    try {
        const response = await gameLiftClient.send(command);
        res.status(200).send(response); 
        return;
    } catch (error) {
        console.error(error);
        res.status(400).send(error);
    }
}
```

### Example: Create Game Session

See the [CreateGameSessionCommand reference](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/Class/CreateGameSessionCommand/).

```js
exports.<your-function-name> = onRequest({region:GCloudRegion},async (req,res)=>{
    if(req.body.CreatorId === undefined){
        res.status(400).send("Missing CreatorId");
        return;
    }
    if(req.body.SessionName === undefined){
        res.status(400).send("Missing SessionName");
        return;
    }

    const input = {
        FleetId: FleetID,
        Location:Location,
        CreatorId:req.body.CreatorId,
        Name:req.body.SessionName,
        MaximumPlayerSessionCount:Number(2),
    };
    const command = new CreateGameSessionCommand(input);
    await executeCommand(res,command);
    return;
});
```

### Example: Terminate Game Session

See the [TerminateGameSessionCommand reference](https://docs.aws.amazon.com/AWSJavaScriptSDK/v3/latest/Package/-aws-sdk-client-gamelift/Class/TerminateGameSessionCommand/).

```js
exports.<your-function-name> = onRequest({region:GCloudRegion},async (req, res) =>{

    if(req.body.GameSessionId === undefined){
        res.status(400).send("Missing GameSessionId");
        return;
    }

    const input = { // TerminateGameSessionInput
        GameSessionId: req.body.GameSessionId, // required
        TerminationMode: "TRIGGER_ON_PROCESS_TERMINATE", // required
    };
    const command = new TerminateGameSessionCommand(input);
    await executeCommand(res,command);
});
```

**Warning:** These examples do not implement any security or authorisation. To protect your service against unauthorised access, you must add your own authentication and authorisation layer.

---

## Unreal Game Client

Connect the game client to your backend service using a C++ HTTP request:

```cpp
void UGLBSServiceConnector::GetSessions(FSearchComplete OnReady)
{	
	TFunction<void(const FJsonObject& Result, const FString& Error)> Done;
	FHttpModule& Module= FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = Module.CreateRequest();

	Request->SetURL("https://<your-region>-<your-gcloud-projectname>.cloudfunctions.net/<your-function-name>");
	Request->SetVerb("GET");
	Request->SetHeader("Content-Type", "application/json");

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
			OnReady.Execute(GameSessions);
			return;
		}
		if (Json->HasField(FString(TEXT("GameSessions"))))
		{
			TArray<FGameSessionData> GameSessionsStruct;
			FGameSessionData data;
			TArray<TSharedPtr<FJsonValue>> GameSessionsJson = Json->GetArrayField(FString(TEXT("GameSessions")));
			for (TSharedPtr<FJsonValue> GameSession : GameSessionsJson)
			{
                //Create a struct or Object from the json result
				data = CreateGameSessionFromJson(GameSession);
				GameSessionsStruct.Add(data);
			}			
			OnReady.Execute(GameSessionsStruct);
			return;
		}
		TArray<FGameSessionData> GameSessions;
		FGameSessionData data;
		GameSessions.Add(data);
		OnReady.Execute(GameSessions);

	});

	Request->ProcessRequest();
}
```

---

## Communication Overview

The following diagram illustrates the GameLift communication flow. Direct game communication between server and client is not shown.

![Communication Overview](Mermaid.jpg)
