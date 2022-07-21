# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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