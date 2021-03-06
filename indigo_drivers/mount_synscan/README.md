# SynScan mount driver

## Supported devices

Any SynScan protocol compatible mount (SkyWatcher; Celestron; Orion; ...) connected over serial port or network.

Single device is present on startup (no hot-plug support).

## Supported platforms

This driver is platform independent.

## License

INDIGO Astronomy open-source license.

## Use

indigo_server indigo_mount_synscan

## Status: Under Development

Driver is developed and tested with:
* SkyWatcher NEQ6 Pro

## Comments

Use URL in form tcp://host:port to connect to the mount over network (default port is 9999).
To export the mount over the network one can use Nexbridge https://sourceforge.net/projects/nexbridge

This driver uses libnexstar library https://sourceforge.net/projects/libnexstar/

A non-standard switch property "Guider rate" is provided by this driver.
