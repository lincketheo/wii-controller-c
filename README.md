Theo's pretty okay bluetooth controller



## Notes
For now, this is a linux only  project. This means there are a few options for windows users. Either, you can re write the code using the windows stl, or you can use virtualization. For the final project, I will have a docker container that will be very simple to use, but for now, the linux requirement is set in stone. 

The key differences between this api and a windows api is accessing serial ports. WIIUSE will work just fine, it is developed to include windows support, however, the windows standard c library does not allow unpriveleged access to serial ports (and they have a wierd scheme). Nothing too hard for a windows developer, however, given I have never written a line of code in windows, I am not up to the task.


##  Build (on linux, replace apt with your package manager, I use arch, and all of these dependencies are met through the AUR)
```bash
# Clone the repo and modules
$ git clone --recursive https://github.com/curmc/wii-controller-c.git && cd wii-controller-c

# Install dependencies
$ make ubuntu-prep # also make arch-prep for arch linux
$ make install-teensy-firmware

# Main make command, builds the executable
$ make # makes libwii library
$ make wiiuse # makes and installs libwiiuse and libwii - do this if you do not know what wiiuse is
$ make exe # makes the executable
$ ./wii-controller-c #  Run the ecxecutable
```

## Some other commands that help in the project
```bash
# Upload to the teensy
$ make upload-teensy # (make sure it's plugged in)

# Clean (remove stuff)
$ make clean

# Install service to systemd
$ sudo make install

# This allows you to start the thing on boot
$ sudo systemctl enable wii-controller-c.service
$ sudo systemctl disable wii-controller-c.service

# Start the systemd unit
$ sudo systemctl start wii-controller-c.service

# Show the status of the systemd unit
$ sudo systemctl status wii-controller-c.service

# Stop the systemd unit
$ sudo systemctl stop wii-controller-c.service

# Uninstall wii controller c
$ sudo make uninstall

# format the code
$ make format
```

## Troubleshooting

1. No bluetooth adapter found
```
.....
17:11:30 INFO  /utils.h:367: Created Wii Controller thread successfully
[ERROR] Could not detect a Bluetooth adapter!
17:11:30 WARN  /robot_control.h:468: No wiimotes found
17:11:30 INFO  /utils.h:367: Created Serial Communication thread successfully

17:11:30 INFO  /utils.h:241: Scanned 10 ports
[ERROR] Could not detect a Bluetooth adapter!
17:11:31 INFO  /utils.h:241: Scanned 10 ports
17:11:31 WARN  /robot_control.h:468: No wiimotes found

17:11:32 INFO  /utils.h:241: Scanned 10 ports
[ERROR] Could not detect a Bluetooth adapter!
17:11:32 WARN  /robot_control.h:468: No wiimotes found
```

This means you do not have bluetooth working. Assuming you are using ubuntu, just open your settings and turn on bluetooth. Pair with the wii remote (pairing and connecting are two different things, the mac address of my wii remote (for those of you on curmc) is 

58:BD:A3:46:92:D4

If you're still having problems, make sure bluetooth is loaded

```bash
$ sudo systemctl start bluetooth
$ sudo systemctl enable bluetooth # to make bluetooth auto loaded at boot time
```

##### Advanced connections

To connect through the terminal, you need to install bluez-utils. First, register keyboard as your agent with default target
```bash
$ bluetoothctl
[bluetooth]# agent KeyboardOnly
Agent registered
[bluetooth]# default-agent
Default agent request successful
```

Then turn on bluetooth and scan and pair with the mac address.
```bash
[bluetooth]# power on
[bluetooth]# scan on
[bluetooth]# pair 58:BD:A3:46:92:D4
[bluetooth]# connect 58:BD:A3:46:92:D4
[bluetooth]# disconnect 58:BD:A3:46:92:D4
```


2. Invalid makefile
This is "beta ish" mode, so I probably have a bunch of mistakes. Just report your errors as issues and I will get back to you as soon as possible.


## API Documentaiton
  For a practical example, see include/kermit.h

##### Callbacks
  This is an implementation of the api put together. This project is completely callback based, so A developer only needs to write a callback to individual methods that they then attach to the robot. A callback is in the form

  ```c
void* button_callback(struct controller_s *controller, struct robot_s *robot);

  ```

  A callback can be anything, in fact it can alter the individual values of the data structure of a robot or controller. Essentially, a callback just changes the data in the two parameters, controller and robot. However, there are a few helper functions in robot. For example, a robot drive train has the method on_vel_callback. These are just here for convinience and could be (but shouldn't be) ignored. I am thinking about adding a register callback method instead of manually declaring the callbacks in kermit_controller, but for now, this will do.


##### Robots
This project is serial / controller independent, thus a wii controlled robot doesn't need to communicate over serial and visa versa. To start the robot, you simply create a robot
```c
robot_main = create_robot();
```
And set it's options
```c++
robot_setopt(&robot_main, DEBUG);
robot_unsetopt(&robot_main, VERBOSE);
... (more options in src/main.c)
```
Then you invoke the thread creater and joiner method with the respective callback
```c
thread__creator( <pthread_t>, <parameter (void*)>, <description (char*)>, <callback function void *(*cb)(void*)>);
thread_joiner( <pthread_t>, <Description (char*)>);
```
