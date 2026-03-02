

# **OdinFleet AWS Gamelift Integration**

This repository provides a reference implementation for integrating ODIN Fleet into Unreal Engine 5, while using AWS GameLift Anywhere and FlexMatch for matchmaking logic and game session management.

The Unreal Engine project provides the C++ and Blueprint logic required to connect your game client to a backend coordination layer (Firebase/Node.js), which in turn manages the communication with AWS and ODIN Fleet.

## Accessing Other Versions

Specific versions are available via the following branches and tags:

* **Autoscaler Integration:** Use the `aws-autoscaler` branch for the [ODIN Fleet Autoscaler guide](https://docs.4players.io/fleet/guides/autoscaler/).
* **FlexMatch Integration:** Use the `aws-flexmatch-matchmaking` branch for the [FlexMatch Integration guide](https://docs.4players.io/fleet/guides/gamelift-flexmatch/).
* **GameLift Anywhere Integration:** Use the `aws-gamelift-anywhere` branch for the [Anywhere Integration guide](https://docs.4players.io/fleet/guides/gamelift-anywhere/).

## Why ODIN Fleet?

ODIN Fleet is the compute device provider in this project because it offers a superior price-performance ratio compared to other providers. It allows you to use the features of GameLift and FlexMatch while running your actual game sessions on ODIN's optimized, low-latency global network.

## More Information and Documentation

For detailed guides on setting up the AWS environment and configuring your ODIN server fleets, check the following:

- [More Information on ODIN Fleet](https://docs.4players.io/fleet/)
- [ODIN Fleet and AWS GameLift Anywhere integration guide](https://docs.4players.io/fleet/guides/gamelift-anywhere/)
- [ODIN Fleet and FlexMatch integration guide](https://docs.4players.io/fleet/guides/gamelift-flexmatch/)
- [ODIN Fleet and Autoscaler integration guide](https://docs.4players.io/fleet/guides/autoscaler/)