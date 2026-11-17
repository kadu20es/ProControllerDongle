# Pro Controller Dongle

[Versão em português](README.md)

A **USB receiver for the Nintendo Switch Pro Controller**, built on a **Raspberry Pi Pico 2 W**.
The Pico talks to the controller over Bluetooth and shows up on the PC as a **native USB Pro Controller**
(`057E:2009`), so the computer's own Bluetooth is not needed. It works with Steam, emulators and any
program that already accepts the Pro Controller over a cable.

```
Pro Controller ──Bluetooth──▶ Raspberry Pi Pico 2 W ──USB──▶ PC
 (buttons, sticks, IMU)         (this firmware)         (sees a Pro Controller)
      ◀── rumble, LEDs ─────────────────◀──────────────────
```

> **Hardware requirement: Raspberry Pi Pico 2 W.**
> The firmware needs the Bluetooth chip (CYW43439) and the **RP2350** USB. The plain **Pico 2** (no "W") has
> **no Bluetooth** and will not work. The original Pico / Pico W (RP2040) is not supported by this firmware.

Independent project, not affiliated with Nintendo, for **personal, non-commercial use** (see [Credits and licenses](#credits-and-licenses)). "Nintendo", "Switch" and "Pro Controller" are trademarks
of their respective owners. Inspired by [DS5Dongle](https://github.com/awalol/DS5Dongle), which does the same
for the DualSense controller.

## Features

- All buttons, both sticks (factory calibration read from the controller itself) and the D-pad.
- **Gyroscope and accelerometer** (IMU) and **HD rumble**, forwarded both ways.
- Player LEDs forwarded from the PC to the controller.
- Automatic reconnection: press any button on the controller.
- **Pairing without the PC**: hold the Pico's BOOTSEL button for 2 s.
- **Wakes the PC from suspend** with a controller button (tested on Windows).
- **Home-button light as an indicator**: dim and steady when connected; slow blink on critical battery.
- Low latency: the dongle adds under 0.5 ms; ~9 ms estimated from Bluetooth input to the PC
  (excluding the controller and the game). See [docs/latencia.md](docs/latencia.md) (Portuguese).
- Two firmwares: **release** (daily use, no serial port) and **debug** (serial log and test commands).

## What you need

| Item | Detail |
|---|---|
| **Raspberry Pi Pico 2 W** | Required (RP2350 + Bluetooth). A micro-USB **data** cable |
| Nintendo Switch Pro Controller | Tested with a genuine one. Clones (8BitDo etc.) were not tested |
| PC | **Windows 11** tested. Linux (`hid-nintendo`) and macOS: not tested |

## Install (users)

1. Download **`ProControllerDongle-vX.Y.Z.uf2`** from the [Releases](../../releases) page.
2. **Hold the BOOTSEL button** on the Pico, **plug it into USB** and release. A drive called **`RP2350`** appears.
3. **Copy the `.uf2`** onto that drive. The Pico reboots by itself. Done.

To update, repeat the steps with the new `.uf2`.

## First use: pair the controller

1. **Remove the controller from Windows Bluetooth** (if it was ever paired there), or it will reconnect to the PC.
2. **Hold BOOTSEL for 2 s.** The board LED blinks fast (pairing mode).
3. **Hold the controller's Sync button** (the small one on top) until the player lights run.
4. When the Pico LED turns **solid**, it is connected. The Home ring lights up dim.

Next time just **press a button** on the controller. The pairing is stored in the Pico's flash.

## Indicators

| Pico board LED | Meaning |
|---|---|
| Solid | Controller connected |
| Fast blink (5 Hz) | Searching for a controller in pairing mode |
| Short pulse every 2 s | Already paired, waiting for the controller (press a button) |

| Home button ring (on the controller) | Meaning |
|---|---|
| Dim, steady | Connected |
| Slow blink (~3.5 s cycle) | Critical battery and not charging |

## Usage notes

- **Steam:** keep **Steam Input** on for the controller. It translates the Pro Controller to XInput, so games
  recognize it and rumble works. Without Steam Input most PC games will not recognize a Pro Controller,
  **just like a genuine controller on a cable**.
- **Gyro in emulators (Eden etc.):** while Steam is open it takes over the controller and the emulator only sees a
  virtual Xbox pad **without gyro**. Close Steam or turn Steam Input off for Switch controllers.
- **Windows `joy.cpl`** shows flickering buttons: this is normal (Windows does not decode the Pro Controller report).
- **Waking the PC:** with the PC asleep, press a controller button. USB power must stay on during suspend
  (a BIOS setting). More in [docs/USO.md](docs/USO.md) (Portuguese).
- Keep the dongle away from USB 3.0 ports and hubs: they add 2.4 GHz noise and increase Bluetooth jitter.

## Known limitations

- **One controller per dongle** (use two dongles for two controllers).
- NFC/IR, controller firmware updates and calibration writes are not supported.
- Linux and macOS were not tested. Pro Controller clones were not tested.
- The USB device is **composite** (Pro Controller + a HID keyboard used only to wake the PC) and uses a 1 ms poll,
  so it is not an exact copy of the original descriptor. Both can be turned off at build time
  ([docs/CONFIGURACAO.md](docs/CONFIGURACAO.md), Portuguese).
- Bluetooth delivers ~67 reports per second; radio jitter (sometimes 30–45 ms) comes from the link, not the dongle.

## Build from source

Summary (details in [docs/COMPILAR.md](docs/COMPILAR.md), Portuguese). You need Pico SDK **2.3.1**, `arm-none-eabi-gcc`,
CMake, Ninja and a host C/C++ compiler (for `pioasm`). On Windows, copy `tools\env.example.cmd` to
`tools\env.cmd`, set your paths and run:

```bat
tools\build.cmd release     rem final firmware -> firmware\build-release\procontroller_dongle.uf2
tools\build.cmd             rem debug firmware -> firmware\build\procontroller_dongle.uf2
```

## Documentation

The detailed documentation is in Portuguese; see the table in [README.md](README.md#documentação).

## Credits and licenses

> **Personal, non-commercial use.** The **code in this repository** is [MIT](LICENSE), but the **compiled firmware
> (`.uf2`) includes BTstack (BlueKitchen) and the CYW43 driver (George Robotics)**, whose licenses allow use and
> redistribution **only for personal, non-commercial purposes**. **Do not sell** dongles or distribute the firmware
> commercially without obtaining the proper commercial licenses. Details and full texts in
> [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md) (Portuguese summary, original license texts).

- The dongle uses **Nintendo's USB VID/PID (`057E:2009`)** only so that Steam, SDL and emulators recognize it
  (interoperability), without authorization from Nintendo or the USB-IF. It is not suitable for products for sale.
- Controller protocol: community reverse engineering, mainly
  [Nintendo_Switch_Reverse_Engineering](https://github.com/dekuNukem/Nintendo_Switch_Reverse_Engineering) (MIT).
- Inspiration and architecture reference: [DS5Dongle](https://github.com/awalol/DS5Dongle) (MIT). No code was copied.
- Libraries: [Raspberry Pi Pico SDK](https://github.com/raspberrypi/pico-sdk) (BSD-3-Clause),
  [TinyUSB](https://github.com/hathach/tinyusb) (MIT), [BTstack](https://github.com/bluekitchen/btstack)
  (own license, non-commercial) and [cyw43-driver](https://github.com/georgerobotics/cyw43-driver)
  (license for Raspberry Pi chips).
