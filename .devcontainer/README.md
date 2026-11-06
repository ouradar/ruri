# UHD Docker

This is a setup to create a developer environment inside a Docker container. It is based on the Ubuntu 24.04 image with a minimal installation to provide basic development environment.

Getting the project

```
git clone https://github.com/ouradar/uhd-docker
```

and a placeholder project `ruri`

```
git clone https://github.com/ouradar/ruri
```

Then, create a symbolic link of `ruri` in `uhd-docker`
```
cd uhd-docker
ln -s ../ruri .
```

To build the base image and launch a container:

```
make
```

To attach to the container:

```
make attach
```

If you try attaching to the container and run the command `tree`, you should get a directory structure like this in the container

![Figure](blob/tree.png)

Congratulations! You are now ready to develop

## Other Useful Commands

To spin down the container:

```
make stop
```

To check if the container exists

```
docker ps
```

Expect to see:
```
CONTAINER ID   IMAGE                    COMMAND          CREATED          STATUS          PORTS     NAMES
8fd2922e03f4   arrc/uhd-docker:latest   "/root/run.sh"   23 minutes ago   Up 22 minutes             uhd-vika
```