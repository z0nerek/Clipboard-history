# Light Clipboard History 📋

A lightweight clipboard manager for Windows, written in C++ using the pure Win32 API.

The primary goal of this project is to provide a fast, reliable clipboard history tool with a minimal memory footprint (approx. 1-2 MB RAM and 0% idle CPU usage). It utilizes the Windows Desktop Window Manager (DWM) to seamlessly blend with the Windows 11 UI, supporting dark mode, rounded corners, and the Acrylic backdrop.

## Features

* **Minimal Resource Usage:** Built with standard Win32 API and an event-driven architecture.
* **Global Hotkey:** Press `Ctrl + B` anywhere to open the clipboard history.
* **Search & Filter:** Instantly filter your clipboard history using the built-in search box.
* **Persistent Storage:** Clipboard history is saved locally and restored after a system reboot.
* **Modern UI:** Integrates with Windows 11 DWM for native dark mode and glass effects.
* **System Tray:** Runs quietly in the background. Right-click the tray icon to exit.
* **Autostart:** Option to automatically run the application on Windows startup.

## Building from Source

1. Clone the repository.
2. Open the `.sln` file in Visual Studio.
3. Ensure the build configuration is set to **Release** for optimal performance.
4. Build the solution (`Ctrl + Shift + B`).

## Usage

1. Run the compiled executable (build it yourself or just download from release). It will minimize to the system tray.
2. Copy text as usual (`Ctrl + C`).
3. Press `Ctrl + B` to open the history menu.
4. Type to filter, use arrow keys or mouse to select, and press `Enter` or double-click to copy an item back to your clipboard.

## 📁 Data Storage

To ensure your clipboard history persists across system reboots, the application saves your copied items locally. The data is stored in a secure, binary format at the following location:

`%LOCALAPPDATA%\ClipboardHistory.dat`

*(Typically resolves to `C:\Users\<YourUsername>\AppData\Local\ClipboardHistory.dat`)*

If you ever want to completely wipe your saved history, you can safely delete this file while the application is closed.
