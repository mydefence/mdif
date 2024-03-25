# Changelog
All notable changes to this repository will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/) and this
repository adheres to [Semantic Versioning](http://semver.org/).

## [1.2.0] - 2024-03-25

### Supported devices
- RF Sensor devices running firmware v2.16.0 or later
- RF Effector devices running firmware v1.5.0 or later

### Added
- Support disabling automatic adjustment of device compass calibration.
- Messages to indicate when automatic adjustments occur in device compass
  calibration, and to persist these changes from client.
- Added fields to GnssStatus, reporting the fix type, number of satellites and
  timing accuracy estimates for GNSS and device system time.
- GetDeviceInfo now reports on the configurability of GNSS on a device.
- Support for subscribing to remote ID messages from RF sensor devices.
- Support for start/stop and querying state of RFE devices

### Changed
- Deprecated the 'configured' field in the GnssStatus message. Note that no RF
  sensor firmware has been released which sets this field.

### Fixed
- Changed precision of some GNSS fields from float to double. Note that this
  breaks compatibility with RF Sensor devices running v2.15.0.
- GnssCompassStreamRes now returns more descriptive errors (code and error
  string) if it fails.

## [1.1.0] - 2024-01-23

### Supported devices
- RF Sensor devices running firmware v2.15.0 or later
- RF Effector devices not supported yet

### Added
- Support for calibration of the compass on devices that have one
- Support for subscribing to a stream of GNSS and compass data
- Android demo project added (RF Sensor v2.14.1 or later)
- HDLC port to Java added (using JNI)

### Changed

### Fixed

## [1.0.0] - 2023-11-22

### Supported devices
- RF Sensor devices running firmware v2.14.1 or later
- RF Effector devices not supported yet

### Added
- Initial files for the repository. Contains documentation, specification and
  examples for MDIF. Included components are: Core, FWU, and RFS (RF Sensor)

### Changed

### Fixed
