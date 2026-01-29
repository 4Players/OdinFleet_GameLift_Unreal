

# **OdinFleet AWS Gamelift Integration**

This repository provides a reference implementation for integrating ODIN Fleet into Unreal Engine 5, while using AWS GameLift Anywhere and FlexMatch for matchmaking logic and game session management.

The Unreal Engine project provides the C++ and Blueprint logic required to connect your game client to a backend coordination layer (Firebase/Node.js), which in turn manages the communication with AWS and ODIN Fleet.

## Accessing Other Versions

Specific versions are available via the following branches and tags:

* **FlexMatch Integration:** Use the `main` branch (tag [`v2-aws-flexmatch-matchmaking`](https://github.com/4Players/OdinFleet_GameLift_Unreal/releases/tag/v2-aws-flexmatch-matchmaking)) for the [FlexMatch Integration guide](https://docs.4players.io/fleet/guides/gamelift-flexmatch/).
* **GameLift Anywhere Integration:** Use the `aws-gamelift-anywhere` branch (tag [`v1-aws-gamelift-anywhere-guide`](https://github.com/4Players/OdinFleet_GameLift_Unreal/releases/tag/v1-aws-gamelift-anywhere-guide)) for the [Anywhere Integration guide](https://docs.4players.io/fleet/guides/gamelift-anywhere/).

## Why ODIN Fleet?

ODIN Fleet is the compute device provider in this project because it offers a superior price-performance ratio compared to other providers. It allows you to use the features of GameLift and FlexMatch while running your actual game sessions on ODIN's optimized, low-latency global network.

## More Information and Documentation

For detailed guides on setting up the AWS environment and configuring your ODIN server fleets, check the following:

- [More Information on ODIN Fleet](https://docs.4players.io/fleet/)
- [ODIN Fleet and AWS GameLift Anywhere integration guide](https://docs.4players.io/fleet/guides/gamelift-anywhere/)
- [ODIN Fleet and FlexMatch integration guide](https://docs.4players.io/fleet/guides/gamelift-flexmatch/)