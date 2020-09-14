
# ASG210_RTApp_W5500_SPI_BareMetal

Real-time (RT) capable application run on bare metal or with a real-time operating system on the real-time cores.
In ASG210, RTApp (Real-time capable Application) is `ASG210_RTApp_W5500_SPI_BareMetal` and it controls WIZnet W5500 ethernet chip and provides variety protocol communications with legacy devices on brown field. Also, it performs inter-core communication between RTApp and HLApp (High-level Application).

`ASG210_RTApp_W5500_SPI_BareMetal` is performed as the followed:

- WIZnet W5500 SPI control
  - Local network communication with brown field
  - Ethernet interface
  - TCP Server for data communication with brown field
  - DHCP Server for local network address configuration of brown field
  - SNTP Server for time information management
- Inter-core communication
  - Send the parsing data from brown field to HLApp

## Build and Run the Application

The application can be run and developed with Visual Studio and Visual Studio Code.

### Run with Visual Studio

Follow these steps to build and run the application with Visual Studio:

1. Start Visual Studio, From the File menu, select `Open` > `Folder` and navigate to the folder, `ASG210_RTApp_W5500_SPI_BareMetal`.

2. From the Select Startup Item menu, on the tool bar, select `GDB Debugger (RTCore)`.

![Visual Studio - Select GDB Debugger](../../Docs/references/visual-studio-select-gdb-debugger-rt.png)

3. Click `Build` > `Build All` to build the project

![Visual Studio - Build the project](../../Docs/references/visual-studio-build-the-project.png)

4. Press <kbd>**F5**</kbd> to start the application with debugging.

### Run with Visual Studio Code

Follow these steps to build and run the application with Visual Studio Code:

1. Open `ASG210_RTApp_W5500_SPI_BareMetal` folder.

![Visual Studio Code - Open Project Folder](../../Docs/references/visual-studio-code-open-project-folder.png)

2. Press <kbd>**F7**</kbd> to build the project

3. Press <kbd>**F5**</kbd> to start the application with debugging
