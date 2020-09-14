
# ASG210_HLApp_AzureIoT

High-level (HL) application run containerized on the Azure Sphere OS. In ASG210, HLApp (High-level application) is `ASG210_HLApp_AzureIoT` and it provides whole functions for Azure IoT Cloud service. Also, it automatically switches global interface, Ethernet and Wi-Fi, for network condition.

ASG210_HLApp_AzureIoT is performed as the followed:

- Global network communication with Azure IoT Cloud service
  - Ethernet and Wi-Fi interface
  - Connection and Authentication on IoT Hub or IoT Central
- Inter-core communication
  - Receive the data from RTApp for sending to Azure IoT Cloud

## Configure an IoT Hub

To operate ASG210 application, RTApp and HLApp, Azure IoT Hub or IoT Central configuration is required.

[Set up an Azure IoT Hub for Azure Sphere](https://docs.microsoft.com/en-us/azure-sphere/app-development/setup-iot-hub)

[Set up an Azure IoT Central to work with Azure Sphere](https://docs.microsoft.com/en-us/azure-sphere/app-development/setup-iot-central)

Then, user will need to supply the following information in the app_manifest.json file for Azure IoT:

- The Tenant ID for ASG210
- The Scope ID for Azure device provisioning service (DPS) instance
- The Azure IoT Hub URL for user IoT Hub or Central along with the global access link to DPS (global.azure-devices.provisiong.net)

In app_manifest.json, add Azure DPS Scope ID, Azure IoT Hub endpoint URL and Azure Sphere Tenant ID from Azure IoT Hub or Central into the following lines:

```
{
  "SchemaVersion": 1,
  "Name": "ASG210_HLApp_AzureIoT",
  "ComponentId": "819255ff-8640-41fd-aea7-f85d34c491d5",
  "EntryPoint": "/bin/app",
  "CmdArgs": [ "<Azure DPS Scope ID>" ],
  "Capabilities": {
    "AllowedConnections": [ "global.azure-devices-provisioning.net", "<Azure IoT Hub endpoint URL>" ],
    "DeviceAuthentication": "<Azure Sphere Tenant ID>",
    "AllowedApplicationConnections": [ "005180bc-402f-4cb3-a662-72937dbcde47" ],
    "Gpio": [
      "$USI_MT3620_BT_COMBO_PIN28_GPIO41",
      "$USI_MT3620_BT_COMBO_PIN27_GPIO42",
      "$USI_MT3620_BT_COMBO_PIN26_GPIO43",
      "$USI_MT3620_BT_COMBO_PIN25_GPIO44",
      "$USI_MT3620_BT_COMBO_PIN24_GPIO45"
    ],
    "NetworkConfig": true,
    "WifiConfig": true
  },
  "ApplicationType": "Default"
}
```

## Set up Public Ethernet interface

To enable ethernet interface for public network and communication with Azure IoT, install ethernet imagepackage by deploying a board configuration image to ASG210. The board configuration image contains information that the Azure Sphere Security Service requires to add support for Ethernet to the Azure Sphere OS.

Follow these steps to enable public ethernet interface:

1. Create a board configuration image package

   ```
   azsphere image-package pack-board-config –-preset lan-enc28j60-isu0-int5 –-output enc28j60-isu0-int5.imagepackage
   ```

2. Prepare ASG210 for development mode

   ```
   azsphere device enable-development
   ```

3. Sideload a board configuration image package

   ```
   azsphere device sideload deploy -–imagepackage enc29j60-isu0-int5.imagepackage
   ```

4. Check the sideloaded imagepackage

   ```
   azsphere device image list-installed
   ```

![Azure Sphere CLI - Image Installed list](../../Docs/references/azure-sphere-cli-image-installed-list.png)

## Build and Run the Application

The application can be run and developed with Visual Studio and Visual Studio Code.

### Run with Visual Studio

Follow these steps to build and run the application with Visual Studio:

1. Start Visual Studio, From the File menu, select Open > Folder… and navigate to the folder, `ASG210_HLApp_AzureIoT`.

2. Open app_manifest.json file and check the information correct.

![Visual Studio - Open app_manifest.json](../../Docs/references/visual-studio-open-app-manifest.josn.png)

3. From the Select Startup Item menu, on the tool bar, select GDB Debugger (HLCore).

![Visual Studio - Select GDB Debugger](../../Docs/references/visual-studio-select-gdb-debugger-hl.png)

4. Click Build>Build All to build the project

![Visual Studio - Build the project](../../Docs/references/visual-studio-build-the-project.png)

5. Press <kbd>**F5**</kbd> to start the application with debugging.

### Run with Visual Studio Code

Follow these steps to build and run the application with Visual Studio Code:

1. Open `ASG210_HLApp_AzureIoT` folder.

![Visual Studio Code - Open Project Folder](../../Docs/references/visual-studio-code-open-project-folder.png)

2. Press <kbd>**F7**</kbd> to build the project

3. Press <kbd>**F5**</kbd> to start the application with debugging
