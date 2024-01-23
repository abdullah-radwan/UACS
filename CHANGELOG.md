# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## Pre-release 6 - 2024-01-23
### Added
- Action areas.
### Changed
- Ingress range are now specified by the vessel.
- Renamed InBreathableArea to InBreathable. Astronauts can specify whether to check atmosphere or not.
- Ability to specify whether the airlock must be open or vessel has at least one empty station in GetNearestAirlock.
- Default astronaut HUD.
### Fixed
- Airlock ground position ignored in Ingress and GetNearestAirlock methods.

## Pre-release 5 - 2023-11-25
### Added
- DrawAstrInfo and DrawCargoInfo methods.
### Fixed
- GetEmptyStationIndex working in reverse, causing several bugs.

## Pre-release 4 - 2023-09-27
### Added
- Astronaut name and role on the astronaut HUD.
- PDB files for UACS DLL modules.
### Changed
- Station and airlock index in TransferAstronaut and EgressAstronaut method is now optional.
- Vessels can now accept/reject astronaut entry (ingress or transfer).
### Fixed
- Ground release logic. It was completely rewritten.
- GetTotalCargoMass causing CTD.

## Pre-release 3 - 2023-08-29
### Added
- Lamp cargo.
- GetStationResources and GetTotalAstrMass methods.
- Module API setup walkthrough for vessels.
### Changed
- Renamed vessel API to module API. The API can be used by Orbiter modules other than vessels.
- AstrInfo className to be the path from 'Config\Vessels' folder (i.e., the same value returned by GetClassNameA).
- Scenario cargo attached to other vessels can no longer be drained from.
- Renamed 'Astronaut' to 'Radwan'.
- DrainResult enum.
- Carrier HUD mode is now saved.
### Fixed
- Astronauts not releasing unpacked cargoes properly.

## Pre-release 2 - 2023-08-10
### Added
- Developer manaul.
- Astronauts can specify suit height and body height.
### Changed
- NearestSearchRange option to SearchRange.
- No vessels outside SearchRange are displayed on astronaut HUD.
- Renamed SaveState to clbkSaveState.
- Renamed DrainUngrappledResource to DrainScenarioResource.
- Renamed station labels.
- InBreathableArea now checks for breathable atmosphere.
- GroundInfo struct.
- If no resource is specified in station config file, the station is assumed to support all resources.
- Cargo options.
- Cargo state saving.
### Fixed
- Movements near the poles.
### Removed
- SetScnAstrInfoByIndex and SetScnAstrInfoByHandle methods.
- Vessel cargo astronaut mode. Replaced by grappleUnpacked and singleObject options.
- Resource container mass.

## Pre-release 1 - 2023-04-27
Too many breaking changes!

## Alpha 5 - 2022-08-02
### Added
- Missing shortcuts to astronaut shortcuts HUD.
### Changed
- ShuttlePB HUD text position.
- ShuttlePB airlock position.
- The API structure to remove all unnecessary call stacking.
- Moved GetNearestBreathable and InBreathableArea from the vessel API to the astronaut API.
### Fixed
- Typo in astronaut config file.
- Incorrect nearest breathable cargo name.

## Alpha 4 - 2022-07-26
### Added
- New API methods for both vessels and astronauts.
### Changed
- All shortcuts to be Alt so as not to interfer with MFD shortcuts. Use Alt + M to change the astronaut HUD mode.
- The API docs were rewritten for better clarity.
### Fixed
- Crashes when the astronaut dies and other issues.
### Removed
- DrainInfo struct and replaced with a pair of DrainResult and drained mass.

## Alpha 3 - 2022-07-21
### Added
- Astronaut is now adjustable via config file.
### Changed
- RCS mode is now forced to off when on ground.

## Alpha 2 - 2022-07-19
### Added
- Proper ShuttlePB HUD and shortcuts for astronaut operations.
- Shortcut HUD modes for astronaut.
- Astronaut to ShuttlePB in orbit scenario.
### Changed
- GetAvailableCargoCount now returns size_t instead of int.
- Operation messages on astronaut and ShuttlePB.
- Shift + I for ingress only works in nearest and vessel HUD modes.
### Fixed
- Cargoes not properly tilted on ground in Moon base scenario.
### Removed
- Flags scenario.

## Alpha 1 - 2022-07-17
Initial release.