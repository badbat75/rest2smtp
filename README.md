# rest2smtp

## Overview
This project exposes a REST API that accepts a JSON message to send an email via the SMTP protocol to an email server. The application can use a queue when specified in the config file.

## Quickstart Guide
1. Clone the repository and navigate to the project directory.
2. Build the project using CMake by running `cmake .` followed by `make`.
3. Install the application by running `make install`.
4. Configure the application by editing the provided example config file.
5. Run the application.

## Build
The project can be built using CMake. Before building the project, make sure that you have the following dependencies installed:
- libconfig
- libmicrohttpd
- json-c
- libcurl
- uuid

To build the project, navigate to the project directory and run `cmake .` followed by `make`.

The build process can be customized by passing the following parameters to the `cmake` command:
- `ENABLE_LTO`: Set this parameter to `ON` to enable Link Time Optimization. The default value is `ON`.
- `USE_SYSTEMD`: Set this parameter to `ON` to use systemd. The default value is `ON`.

For example, to build the project with Link Time Optimization enabled and systemd disabled, run the following command: `cmake -DENABLE_LTO=ON -DUSE_SYSTEMD=OFF .`

## Installation
After building the project, it can be installed by running `make install`. This will install the application and its associated files to their appropriate locations.

## Configuration
The application can be configured by editing the config file located at `${CMAKE_INSTALL_PREFIX}/etc/rest2smtp/rest2smtp.conf`. The available configuration options are:
- **HTTP Server configuration**: Set the HTTP and HTTPS ports, and specify the paths to the certificate and key files for HTTPS.
- **Queue configuration**: Set the path to the queue and enable or disable the use of a queue.
- **SMTP Server configuration**: Set the SMTP server address, username, and password.
