# NIT

[![License](https://www.gnu.org/graphics/gplv3-88x31.png)](link-to-license)

Nit is a backlight manager for screen and keyboard.

## Understanding
Nit uses a controller for each device in order to manage brightness.

### Screen
The screen controller is loaded from `/sys/class/backlight` and it is by
default `nv_backlight`. This value can be overwritten setting the environment
variable `$NIT_CTRL_SCREEN`.

### Keyboard
The keyboard controller is loaded from `/sys/class/leds` and it is by default
`smc::kbd_backlight`. This value can be overwritten setting the environment
variable `$NIT_CTRL_KEYBOARD`.

## Installing
1. First step consists in downloading, compiling and installing the source:
``` shell session
$ git clone https://github.com/matteocellucci/<proj>
$ cd nit
$ make clean && make && sudo make install
```
2. In the second step you may need to set the enviroment variables accordingly
with your controllers name. You can find them listing files in their loadings
folder:
``` shell session
$ export $NIT_CTRL_SCREEN="<screen_ctrl_name>"
$ export $NIT_CTRL_KEYBOARD="<keyboard_ctrl_name>"
```
_*Warning*_: besides a single instance execution, in order to make this settings
persistent after reboot, you should add these commands in a file such
`rc.local`.

3. The command is ready but it can be run only with `sudo` permissions. To
allow a user to use it, a further step is needed. Execute:
``` shell session
$ sudo nit --setup
```
_*Warning"_: to make changes available you must achieve a reboot or at least a 
login/logout.

## Uninstalling
I really like feedbacks of any kind. Feel free to open issues or to contact me
in any way. Simply:
``` shell session
$ cd nit
$ make unistall
```
I hope I'll see you again in the future!

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
Contributions to Nit are greatly appreciated, whether a feature request or a
bug report. You can make magic even by yourself, forking Nit. I'd enjoy if you
will request a pull!
 
