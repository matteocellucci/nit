# NIT

[![License GPLV3](https://www.gnu.org/graphics/gplv3-88x31.png "License GPLv3")
](https://github.com/matteocellucci/nit/blob/master/LICENSE)

Nit is a backlight manager for screen and keyboard.

## Understanding
Nit uses a different controller for each device that can handle the brightness.

The screen controller is loaded from `/sys/class/backlight` and it is by
default `nv_backlight`. This value can be overwritten setting the environment
variable `$NIT_CTRL_SCREEN`.

The keyboard controller is loaded from `/sys/class/leds` and it is by default
`smc::kbd_backlight`. This value can be overwritten setting the environment
variable `$NIT_CTRL_KEYBOARD`.

## Installing
The first step consists in downloading, compiling and installing the source:
``` shell session
$ git clone https://github.com/matteocellucci/nit.git
$ cd nit
$ make clean && make && sudo make install
```
In the second step you may need to set the enviroment variables accordingly
with your controllers name. You can find them listing files in their loadings
folder:
``` shell session
$ export $NIT_CTRL_SCREEN="<screen_ctrl_name>"
$ export $NIT_CTRL_KEYBOARD="<keyboard_ctrl_name>"
```
**Warning**: besides current session, in order to make these settings
persistent after reboot, you should add those commands in a file such
`rc.local`.

The installation is complete, but Nit can run only with `sudo` permission due
to the mode set of the controller. To allow for the logged user to use it a
further step is needed. Execute:
``` shell session
$ sudo nit --setup
```
**Warning**: to make changes available you must fullfill a reboot or at least a 
login/logout.

## Uninstalling
Simply:
``` shell session
$ cd nit
$ make unistall
```

## Usage
```
nit [DEVICE] [OPTION]

Option:
  -h, --help             display this help and exit
  -l, --list             list controllers names
      --setup            need root permission; generate rules to control
                         brightness; this option will be executed first; see
                         PERMISSIONS for more details
  -s [VAL], --set=[VAL]  adjust brightness according to VAL; VAL is an integer
                         and can be formatted in three ways. Each of them is
                         meaningfull:
                             VAL    set VAL as current brightness
                            +VAL    add VAL to current brightness
                            -VAL    sub VAL from current brightness
  -S, --silent-mode      don't print feedback brightness value after '-s'
  -v, --version          output version information and exit

Device:
  --screen               select screen controller
  --keyboard             select keyboard controller
```

## Example
### Add 30 points of brightness to the screen
``` shell session
$ nit --screen -s +30
```
### Set to 200 the keyboard brightness
``` shell session
$ nit --keyboard -s 200
```
### Subtract 4 points of brightness to the screen
``` shell session
$ nit --screen -s -4
```

## Contribution
Contributions to Nit are greatly appreciated, whether it's a feature request or
a bug report. You can make magic trick even by yourself. I'll enjoy if you
request a pull!
