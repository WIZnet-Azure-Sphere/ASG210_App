## Sample: I2C_LSM6DS3_HighLevelApp

This sample C application demonstrates how to use [I2C with Azure Sphere](https://docs.microsoft.com/azure-sphere/app-development/i2c) in a high-level application. By the input of user button, the sample starts sending 10 character data and receives 10 character data, through I2C (Inter-Integrated Circuit).

[Applibs I2C APIs](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-i2c/i2c-overview)
[Log_Debug](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-log/function-log-debug).

To run the sample using the ASG210 and the other device which has I2C with slave address 8. The other device should receive 10 character data from master and send 10 character data to master.

The sample uses the following Azure Sphere libraries:

|Library   |Purpose  |
|---------|---------|
|log     |  Displays messages in the Visual Studio Device Output window during debugging  |
|I2C   | Manages I2C interfaces |
|GPIO    | Manages user button and Port of I2C SDA |
|TIMER    | Manages Poll of input user button |
| [eventloop](https://docs.microsoft.com/azure-sphere/reference/applibs-reference/applibs-eventloop/eventloop-overview) | Invoke handlers for timer events |

## Prerequisites

 This sample requires the following hardware:

- WIZnet ASG210![Azure Sphere Guardian 210](https://doc.wiznet.io/img/AzureSphere/ASG210_board_description.png)

- Board which has I2C with salve address 8

- Jumper wires to connect the boards.

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

1. Start Visual Studio, From the File menu, select Open > Folder… and navigate to the folder, `I2C_HLApp_mt3620_BareMetal`.

2. Open app_manifest.json file and check the information correct.

![Visual Studio - Open app_manifest.json](../../Docs/references/visual-studio-open-app-manifest.josn.png)

3. From the Select Startup Item menu, on the tool bar, select GDB Debugger (HLCore).

![Visual Studio - Select GDB Debugger](../../Docs/references/visual-studio-select-gdb-debugger-hl.png)

4. Click Build>Build All to build the project

![Visual Studio - Build the project](../../Docs/references/visual-studio-build-the-project.png)

5. Press <kbd>**F5**</kbd> to start the application with debugging.

### Run with Visual Studio Code

Follow these steps to build and run the application with Visual Studio Code:

1. Open `I2C_HLApp_mt3620_BareMetal` folder.

![Visual Studio Code - Open Project Folder](../../Docs/references/visual-studio-code-open-project-folder.png)

2. Press <kbd>**F7**</kbd> to build the project

3. Press <kbd>**F5**</kbd> to start the application with debugging

## Observe the output

 Press user button to send 10 character data to other device and receives 10 character data from other device which has I2C with slave address 8. The other device should receive 10 character data from master and sends 10 character data to master.

You will need the component ID to stop or start the application. To get the component ID, enter the command `azsphere device app show-status`. Azure Sphere will return the component ID (a GUID) and the current state (running, stopped, or debugging) of the application.

```sh
C:\Build>azsphere device app show-status
12345678-9abc-def0-1234-a76c9a9e98f7: App state: running
```

To stop the application enter the command `azsphere device app stop -i <component ID>`.

To restart the application enter the command `azsphere device app start -i <component ID>`.
