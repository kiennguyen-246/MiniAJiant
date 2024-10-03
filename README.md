# MiniAJiant

## Introduction

MiniAJiant is a small project for the Viettel Digital Talent 2024 program. Aiming to solve the APT attack problem, MiniAJiant provides a simple EDR (Endpoint Detection & Response) solution. It will filter and detect suspicious process creating events on endpoint computers within a network.

The project consists of three major components: server, agent user-mode application, and agent driver. The agent driver is the core component of the project. It implements [Minifilter](https://learn.microsoft.com/en-us/windows-hardware/drivers/ifs/filter-manager-concepts), the latest kernel-mode file system driver technology, to catch any process creating operations taking place after the installation of the driver. The agent user-mode application receive events from the kernel mode driver, exploit as much information of the process as possible, then send them to the server. Finally, the server implements [WebSocket](https://websocket.org/guides/road-to-websockets/#:~:text=A%20WebSocket%20connection%20starts%20as%20an%20HTTP%20request/response%20handshake;%20beyond) to receive events from the agent applications installed in endpoint computers, then try to detect dangerous event using [YARA rules](https://www.bing.com/search?pglt=43&q=yara+docs&cvid=466ee886db124e5b96840d91b0f10daf&gs_lcrp=EgZjaHJvbWUyBggAEEUYOTIGCAEQABhAMgYIAhAuGEAyBggDEC4YQDIGCAQQLhhAMgYIBRAAGEAyBggGEAAYQDIGCAcQABhAMgYICBAAGEDSAQgxNjMxajBqMagCALACAA&FORM=ANNTA1&PC=U531). All of the events are listed out in the console windows, including warning events.

The server of MiniAJiant can run on any Debian OS, while the client agent can run on Windows of versions later than Windows 10 only.

**Important note**: This project is still in development, therefore a lot of steps are considered "unsafe" by Windows. Please carefully follow the steps to ensure your device's safety.

## Installation

### Server

Download the tarball from the [Release](https://github.com/kiennguyen-246/MiniAJiant/releases/tag/v0.1.0) section of this repo:

```shell
curl -L https://github.com/kiennguyen-246/MiniAJiant/releases/download/v0.1.0/MiniAJiantServer.tar.gz --output MiniAJiantServer.tar.gz
```

Extract the tarball
```shell
tar -xf MiniAJiantServer.tar.gz
```

Start the server using the following command
```shell
./server [host address] [host port]
```

### Client

First, download the zipped file (MiniAJiant.zip) from the [Release](https://github.com/kiennguyen-246/MiniAJiant/releases/tag/v0.1.0) section of this repo. Extract the zipped file.

Next, you have to enable test mode in order to use the driver. In Windows, driver must have a digital signature to be allowed to be installed. Obtaining a licensed signature is a tedious process, so for simplicity we temporarily use Test mode for this driver. Test mode enable installing drivers with test signature, but not unsigned drivers.

To enable test mode, first you have to disable Secure Boot. Secure Boot can be disabled n UEFI or BIOS menu, which is accessible by pressing F1, F2, F12 or Esc while the computer is starting up. After the computer has successfully started, open a command line terminal anywhere and type:

```cmd
bcdedit -set TestSigning On
```

**Important note**: Disabling Secured Boot and enabling Test mode put your computer at high risk. You should set Secure Boot back to On and Test mode to Off as soon as you finish installing the driver.

After enabling Test mode, restart the computer for the changes to take effect.

When going back, open a command line at the directory where you extracted the zipped file. Run the following command line
```cmd
.\user [host address] [host port]
```
where `[host address]`, `[host port]` indicates the address of the server. The application would not run if you use other methods.

A command shell will be opened. Run `install` to install the driver. A warning dialogue will appear, warn you that the driver you are installing may harm the computer. Choose to install it anyway (trust me, this driver is safe).

When the installing process is complete, run `start` to load the driver and start filtering. You may see the result in `logs` folder, or check the output command line on the server.

If you wish to stop filtering, run `stop` on the command shell. After a while, the driver will be unloaded, and no more message will be send to the server.