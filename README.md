# A Minimal Docker image with a C++ gRPC server.

This project shows how to create a minimal Docker image containing a C++ gRPC
service.

## Requirements

### Operating System

This demo is intended for Linux systems only. It may work on other operating
systems using some form of virtual machine, but I have not tested it.

### Docker

You must have [Docker][docker-link] installed on your workstation. I have only
tested the Docker files with 18.09, but any version greater than 17.05 should
work.

## Create devtools Docker image

First we create a Docker image with all the development tools necessary to
compile the gRPC server. While it is possible to install all these development
tools in your workstation, a Docker image makes the remaining steps easier to
reproduce.

This step may take a few minutes, even a couple of hours, as it builds gRPC and other dependencies. For reference, it took 1h:50m to build the image while running in a development laptop configured with a Core i5 8th Gen processor and 16GB of RAM.

```bash
sudo docker build -t grpc-cpp-devtools:latest -f tools/Dockerfile.devtools tools
```

## Create the server Docker image

Once the development tools image is created we can use it to create a Docker
image with a C++ gRPC server:

```bash
sudo docker build -t grpc-cpp-echo:latest -f examples/echo/Dockerfile.server .
```

Note that this image is created for the echo example. Besides, you can easily build images for other examples you place into the `examples` directory by using the same naming pattern of the **echo** example. For instance, we have built another, **filetransfer** example whose server you can built with the command below:

```bash
sudo docker build -t grpc-cpp-filetransfer:latest -f examples/filetransfer/Dockerfile.server .
```

However, we continue only by building the **echo** example image to keep it simple.

Note also that the images are relatively small:

```bash
sudo docker image ls grpc-cpp-echo:latest
```

```console
REPOSITORY          TAG                 IMAGE ID            CREATED             SIZE
grpc-cpp-echo       latest              04d95e5adaa6        4 minutes ago       14.6MB
```

## Run the server in the Docker image

For general behavior, use `docker run` to start a container using this image. You may want to detach
from the image using the `-d` option and capture its id so you can terminate it
later:

```bash
ID=$(sudo docker run -d -P grpc-cpp-echo:latest /r/echo_server)
```

### Using bind mounts

Note that the `filetransfer` example might use the `files` folder in the root of the project as a bind mount directory. This uses a synchronized (two-ways directory) that you can run as below in **Windows PowerShell**:

```powershell
docker run -d -P -v "${PWD}\files:/app/files" grpc-cpp-filetransfer:latest /r/filetransfer_server
```

Note the mapping of port 7000 to the localhost to ease testing.

## Use the client to test connectivity

The image also contains a small client to demonstrate connecting to it:

```bash
ADDRESS=$(sudo docker port "${ID}" 7000)
sudo docker run --network=host grpc-cpp-echo:latest /r/echo_client --address "${ADDRESS}"
```

## Terminate the container

```bash
sudo docker kill "${ID}"
```

[docker-link]: https://www.docker.com/
