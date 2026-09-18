# USB Mass Storage design boundary

USB Mass Storage is exclusive ownership of a block device. Before starting it:

1. Return to the launcher and stop all app-owned workers.
2. Close every file and unmount the SD card.
3. Start a TinyUSB MSC adapter which owns the block device.
4. On disconnect, stop MSC and remount the SD card.

Do not allow the firmware and a USB host to mount/write the same FAT filesystem at the same time.

The PaperS3 microSD slot is SPI-connected, so expose it through a custom TinyUSB block backend. The ESP-IDF ready-made SDMSC helper targets SDMMC rather than this board's SPI wiring.
