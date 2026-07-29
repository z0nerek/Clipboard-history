# Light Clipboard History 📋

A lightweight clipboard manager for Windows, written in C++ using the pure Win32 API.

The primary goal of this project is to provide a fast, reliable clipboard history tool with a minimal memory footprint (approx. 1-2 MB RAM and 0% idle CPU usage)[cite: 1]. It utilizes the Windows Desktop Window Manager (DWM) to seamlessly blend with the Windows 11 UI, supporting dark mode, rounded corners, and the Acrylic backdrop[cite: 1].

## Features

* **Minimal Resource Usage:** Built with standard Win32 API and an event-driven architecture[cite: 1].
* **Global Hotkey:** Press `Ctrl + B` anywhere to open the clipboard history[cite: 1].
* **Image Clipboard Support:** Automatically captures copied images (`CF_BITMAP` / `CF_DIB`), saves them locally, and features a dynamic preview panel on the left that appears only when an image item is selected.
* **Item Pinning:** Select any item and press `Insert` to pin it (`[PIN]`). Pinned items stay safely at the top and are protected from automatic deletion or clearing.
* **Search & Filter:** Instantly filter your text and image history using the built-in search box[cite: 1] with full `Ctrl + A` text selection support.
* **Smart Tray Cleanup:** Use the tray menu option "Clear History (Keep Pins)" to wipe unpinned clutter while keeping your favorite items intact.
* **Quick Hide:** Press `ESC` anywhere in the app to instantly minimize/hide it to the system tray.
* **Persistent Storage:** Clipboard history and images are saved locally and restored after a system reboot[cite: 1].
* **Modern UI:** Integrates with Windows 11 DWM for native dark mode and glass effects[cite: 1].
* **System Tray:** Runs quietly in the background[cite: 1]. Right-click the tray icon to clear history or exit[cite: 1].
* **Autostart:** Option to automatically run the application on Windows startup[cite: 1].

## Building from Source

1. Clone the repository[cite: 1].
2. Open the `.sln` file in Visual Studio[cite: 1].
3. Ensure the build configuration is set to **Release** for optimal performance[cite: 1].
4. Build the solution (`Ctrl + Shift + B`)[cite: 1].

## Usage

1. Run the compiled executable[cite: 1]. It will minimize to the system tray[cite: 1].
2. Copy text or images as usual (`Ctrl + C`).
3. Press `Ctrl + B` to open the history menu[cite: 1].
4. Type to filter, use arrow keys or mouse to select, press `Insert` to pin/unpin, press `ESC` to hide, or double-click to copy an item back to your clipboard[cite: 1].

## 📁 Data Storage

To ensure your clipboard history persists across system reboots, the application saves your copied items locally[cite: 1]. The text history is stored in a secure, binary format at the following location[cite: 1]:

`%LOCALAPPDATA%\ClipboardHistory.dat`

*(Typically resolves to `C:\Users\<YourUsername>\AppData\Local\ClipboardHistory.dat`)[cite: 1]*

Images are stored locally in `%LOCALAPPDATA%\ClipboardHistory_Images\`. If you ever want to completely wipe your saved history, you can safely delete these files while the application is closed[cite: 1].