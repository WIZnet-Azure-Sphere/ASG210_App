# Sample: GPIO_HLApp_mt3620_BareMetal (High-level app)

This application does the following:

- Provides access to one of the LEDs on the WIZnet ASG210 using GPIO
- Uses a button to change the blink rate of the LED

The sample uses the following Azure Sphere libraries.

| Library | Purpose |
|---------|---------|
| [GPIO](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-gpio/gpio-overview) | Manages user button and LED 1 on the device |
| TIMER | Manages Poll of input user button |
| [log](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/log-overview) | Displays messages in the Visual Studio Device Output window during debugging |
| [EventLoop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Contents

| File/folder | Description |
|-------------|-------------|
| GPIO_HLApp_mt3620_BareMetal       |Sample source code and VS project files |
| README.md | This readme file |

## Prerequisites

The sample requires the following hardware:

![Azure Sphere Guardian 210](https://doc.wiznet.io/img/AzureSphere/ASG210_board_description.png)

## Prepare the sample

1. Ensure that your Azure Sphere device is connected to your computer, and your computer is connected to the internet.
1. Even if you've performed this setup previously, ensure that you have Azure Sphere SDK version 20.07 or above. At the command prompt, run **azsphere show-version** to check. Install the Azure Sphere SDK for [Windows](https://docs.microsoft.com/azure-sphere/install/install-sdk) or [Linux](https://docs.microsoft.com/azure-sphere/install/install-sdk-linux) as needed.
1. Enable application development, if you have not already done so, by entering the following line at the command prompt:

   `azsphere device enable-development`

1. Clone the [ASG210_App](https://github.com/WIZnet-Azure-Sphere/ASG210_App) repo and find the GPIO_HighLevelApp sample in the Software folder.

## Build and run the sample

The application can be run and developed with Visual Studio and Visual Studio Code.

### Run with Visual Studio

Follow these steps to build and run the application with Visual Studio:

1. Start Visual Studio, From the File menu, select Open > Folder… and navigate to the folder, `GPIO_HLApp_mt3620_BareMetal`.

2. Open app_manifest.json file and check the information correct.

![Visual Studio - Open app_manifest.json](../../Docs/references/visual-studio-open-app-manifest.josn.png)

3. From the Select Startup Item menu, on the tool bar, select GDB Debugger (HLCore).

![Visual Studio - Select GDB Debugger](../../Docs/references/visual-studio-select-gdb-debugger-hl.png)

4. Click Build>Build All to build the project

![Visual Studio - Build the project](../../Docs/references/visual-studio-build-the-project.png)

5. Press <kbd>**F5**</kbd> to start the application with debugging.

### Run with Visual Studio Code

Follow these steps to build and run the application with Visual Studio Code:

1. Open `GPIO_HLApp_mt3620_BareMetal` folder.

![Visual Studio Code - Open Project Folder](../../Docs/references/visual-studio-code-open-project-folder.png)

2. Press <kbd>**F7**</kbd> to build the project

3. Press <kbd>**F5**</kbd> to start the application with debugging

## Observe the output

 LED1 AZURE on the WIZnet ASG210 begins blinking.

 Press user button repeatedly to cycle through the 3 possible blink rates.

You will need the component ID to stop or start the application. To get the component ID, enter the command `azsphere device app show-status`. Azure Sphere will return the component ID (a GUID) and the current state (running, stopped, or debugging) of the application.

```sh
C:\Build>azsphere device app show-status
12345678-9abc-def0-1234-a76c9a9e98f7: App state: running
```

To stop the application enter the command `azsphere device app stop -i <component ID>`.

To restart the application enter the command `azsphere device app start -i <component ID>`.
