/* nit is a brightness manager for keyboard and screen.
   Copyright (C) 2017 Matteo Cellucci

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <ctype.h>
#include <grp.h>
#include <unistd.h>

#define PROGRAM_NAME "nit"
#define AUTHOR_NAME "Matteo Cellucci"
#define VERSION "1.0"
#define GROUP_NAME "nit-group"
#define RULES_DIR "/etc/udev/rules.d/99-nit.rules"
#define SUBSYSTEM_NAME "backlight"
#define ACTION_NAME "add"

/* Types of exit status:
     0 - successfully ended;
     1 - catchall for general problems;
     2 - command misuse (e.g. permission problem or invalid option);
     3 - embarassing situation.  */
enum exit_status
{
  success,
  failure,
  misuse,
  this_is_embarassing
};

/* Types of brightness with relatives files name:
     0 - current brightness;
     1 - minimum brightness;
     2 - maximum brightness.  */
enum bness_type
{
  current,
  min,
  max
};

/* Types of brightness variation:
     0 - no variations;
     1 - the variation must be added to the brightness;
     2 - the variation must be subtracted to the brightness;
     3 - the variation is the brightness itself.  */
enum bness_delta_type
{
  none,
  positive,
  negative,
  absolute
};

/* Types of controller:
     0 - screen controller;
     1 - keyboard controller.  */
enum controller_type
{
  screen,
  keyboard
};

/* A controller manages the brightness of the associated device.  */
struct controller
{
  char *dir;          // controller path.
  char *name;         // controller name.
  int current_bness;  // current brightness value.
  int min_bness;      // minimum brightness value.
  int max_bness;      // maximum brightness value.
};

/* Current status of the process.  */
static enum exit_status exit_status;

/* Sign of the variation (-s). */
static enum bness_delta_type bness_delta_type;

/* Value of the variation (-s arg).  */
static int bness_delta_value;

/* Censure any feedback on stdout after brighntess variation (-S).  */
static int silent_mode;

/* Configure rules to execute the command without sudo (-R).  */
static int setup_mode;

/* Print current controllers configuration (-l).  */
static int print_controllers;

/* Active controller and controllers set:
     controllers[0] - screen controller (--screen);
     controllers[1] - keyboard controller (--keyboard).  */
static struct controller *controller;

static struct controller controllers[] =
{
  {"/sys/class/backlight", "nv_backlight", 0, 0, 0},
  {"/sys/class/leds", "smc::kbd_backlight", 0, 0, 0}
};

/* Option list: --screen, --keyboard and --setup have a pseudo short option in
   order to complete the parsing.  */
enum pseudo_options
{
  screen_opt,
  keyboard_opt,
  setup_opt
};

static struct option const long_options[] =
{
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  {"setup", no_argument, NULL, setup_opt},
  {"list", no_argument, NULL, 'l'},
  {"set", required_argument, NULL, 's'},
  {"silent-mode", no_argument, NULL, 'S'},
  {"screen", no_argument, NULL, screen_opt},
  {"keyboard", no_argument, NULL, keyboard_opt},
  {NULL, 0, NULL, 0}
};

static void parse_options (int argc, char *argv[]);
static void controller_start ();
static int controller_get_bness (const enum bness_type type);
static void controller_set_bness ();
static void update_controllers ();
static void list_controllers ();
static void rules_setup ();
static char * generate_rule (const char *command,
                             const enum controller_type type);
static void throw_error (const char *message, const enum exit_status code);
static void check_failure (const int result, const char *message);
static void usage ();
static void version ();

int
main (int argc, char *argv[])
{
  exit_status = success;
  
  parse_options (argc, argv);

  if (setup_mode)
    {
      rules_setup ();
    }
  if (print_controllers)
    {
      list_controllers ();
    }
  if (controller != NULL)
    {
      controller_start ();
      
      if (bness_delta_type == none)
        {
          printf ("%d/%d\n",
                  controller->current_bness - controller->min_bness,
                  controller->max_bness - controller->min_bness);
        }
      else
        {
          if (bness_delta_type == positive)
            {
              controller->current_bness += bness_delta_value;
            }
          else if (bness_delta_type == negative)
            {
              controller->current_bness -= bness_delta_value;
            }
          else
            {
              controller->current_bness = bness_delta_value;
            }
    
          if (controller->current_bness > controller->max_bness)
            {
              controller->current_bness = controller->max_bness;
            }
          else if (controller->current_bness < controller->min_bness)
            {
              controller->current_bness = controller->min_bness;
            }

          controller_set_bness ();
          
          if (!silent_mode)
            {
              printf ("%d\n", controller->current_bness);
            }
        }
    }

  return exit_status;
}

/* Parse options. There are two kind of option: slow and fast. Slow options are
   those who set a flag and they can be evaluated togheter, fast options are
   executed immediatly, stopping parsing and terminating the process. These are
   --help and --version options.  */
static void
parse_options (int argc, char *argv[])
{
  /* optarg temp conteiner.  */
  char *oa = NULL;

  bness_delta_type = none;
  bness_delta_value = 0;
  silent_mode = 0;
  setup_mode = 0;
  print_controllers = 0;
  controller = NULL;
  
  if (argc <= 1)
    {
      usage();
    }
 
  update_controllers();
  
  while (1)
    {
      int oi = -1;
      int c = getopt_long (argc, argv, "hvRlgs:", long_options, &oi);
      if (c == -1)
        {
          break;
        }
      switch (c)
        {
          case 'h':
            usage ();
            break;
          case 'v':
            version ();
            break;
          case 'l':
            print_controllers = 1;
            break;
          case 's':
            if (optarg[0] == '+')
              {
                bness_delta_type = positive;
                oa = optarg + 1;
              }
            else if (optarg[0] == '-')
              {
                bness_delta_type = negative;
                oa = optarg + 1;
              }
            else if (isdigit(optarg[0]))
              {
                bness_delta_type = absolute;
                oa = optarg;
              }
            else
              {
                throw_error ("invalid argument '-s'", misuse);
              }
            for (unsigned int i = 1; i < strlen (optarg); i++)
              {
                if (!isdigit (optarg[i]))
                  {
                    throw_error ("invalid argument '-s'", misuse);
                  }
              }
            bness_delta_value = (int) strtol (oa, (char **)NULL, 10);
            break;
          case 'S':
            silent_mode = 1;
            break;
          case setup_opt:
            setup_mode = 1;
            break;
          case screen_opt:
            controller = &controllers[screen];
            break;
          case keyboard_opt:
            controller = &controllers[keyboard];
            break;
          default:
            exit_status = misuse;
            usage ();
            break;
        }
    }
  
  if (bness_delta_type != none && controller == NULL)
    {
      throw_error ("missing or unknow controller", misuse);
    }
}

/* Start the controller, initializing brightness values.  */
static void
controller_start ()
{
  controller->min_bness = controller_get_bness (min);
  controller->max_bness = controller_get_bness (max);
  controller->current_bness = controller_get_bness (current);
}

/* Get a specific brightness value.  */
static int
controller_get_bness (const enum bness_type type)
{
  int cd;
  int error_flag;
  char *bness_val;
  char *bness_path;
  char *bness_file;
  size_t bness_val_len;
  size_t bness_path_len;

  bness_val_len = 4;
  bness_path_len = strlen (controller->dir) + strlen (controller->name) + 1;
  bness_val = malloc (bness_val_len * sizeof (char));
  bness_path = NULL;
  bness_file = NULL;

  if (type == max)
    {
      bness_file = "max_brightness";
    }
  else if (type == current)
    {
      bness_file = "brightness";
    }
  else // if (type == min)
    {
      free (bness_val);
      return 0;
    }

  bness_path_len = bness_path_len + strlen ("//") + strlen (bness_file);
  bness_path = malloc (bness_path_len * sizeof (char));
  error_flag = snprintf (bness_path, bness_path_len * sizeof (char),
                         "%s/%s/%s", controller->dir, controller->name,
                         bness_file);
  check_failure (error_flag, "unable to fetch controller's path");
  
  cd = open (bness_path, O_RDONLY, S_IRUSR | S_IRGRP);
  check_failure (cd, "controller not found or permission denied");
  error_flag = read (cd, bness_val, bness_val_len * sizeof (char));
  check_failure (error_flag, "unable to read current brightness");
  error_flag = close(cd);
  check_failure (error_flag, "unable to read current brightness");

  int bness_val_buffer = (int) strtol (bness_val, (char **) NULL, 10);
  free (bness_val);
  free (bness_path);
  bness_val = NULL;
  bness_path = NULL;
  return bness_val_buffer;
}

/* Make the new brightness active.  */
static void
controller_set_bness ()
{
  int cd;
  int error_flag;
  char *bness_val;
  char *bness_path;
  size_t bness_val_len;
  size_t bness_path_len;

  bness_val_len = sizeof (int) + sizeof (char);
  bness_path_len = strlen (controller->dir) + strlen (controller->name)
                   + strlen ("//brightness") + 1;
  bness_val = malloc (bness_val_len * sizeof (char));
  bness_path = malloc (bness_path_len * sizeof (char));

  error_flag = snprintf (bness_path, bness_path_len * sizeof (char),
                         "%s/%s/brightness", controller->dir,
                         controller->name);
  check_failure (error_flag, "unable to fetch controller's path");
  cd = open (bness_path, O_WRONLY, S_IWUSR | S_IWGRP);

  check_failure (cd, "controller not found or permission denied");
  error_flag = snprintf (bness_val, bness_val_len * sizeof (char), "%d",
                         controller->current_bness);
  check_failure (error_flag, "unable to fetch new brightness value");
  error_flag = write (cd, bness_val, bness_val_len * sizeof (char));
  check_failure (error_flag, "unable to write new brightness");
  error_flag = close (cd);
  check_failure (error_flag, "unable to write new brightness");

  free (bness_val);
  free (bness_path);
  bness_val = NULL;
  bness_path = NULL;
}

/* Load controllers paths from the environment.  */
static void
update_controllers ()
{ 
  char *sname = getenv ("NIT_CTRL_SCREEN");
  char *kname = getenv ("NIT_CTRL_KEYBOARD");

  if (sname != NULL)
    {
      controllers[screen].name = sname;
    }
  if (kname != NULL)
    {
      controllers[keyboard].name = kname;
    }
}

/* List current controller's names.  */
static void
list_controllers ()
{
  printf ("Screen: %s\n", controllers[screen].name);
  printf ("Keyboard: %s\n", controllers[keyboard].name);
}

/* Setup rules in order to permit execution without sudo.  */
static void
rules_setup ()
{
  int cd;
  int error_flag;
  int is_grp;
  char *command;
  char *username;
  char **rules;
  char **members;
  struct group *grp;
  size_t command_len;

  command = NULL;
  // check sudo permission
  if (getuid() != 0)
    {
      throw_error("permission denied", misuse);
    }
  
  // check group existence
  grp = getgrnam(GROUP_NAME);
  username = getlogin();
  if (grp == NULL)
    {
      // create the group and add the user
      command_len = strlen (GROUP_NAME) * 2 + strlen (username)
                    + strlen ("groupadd ; usermod -a -G  ") + 1;
      command = malloc (command_len * sizeof (char));
      error_flag = snprintf (command, command_len * sizeof (char),
                            "groupadd %s; usermod -a -G %s %s", GROUP_NAME,
                            GROUP_NAME, username);
      check_failure (error_flag, "unable to fetch group commands");
      error_flag = system (command);
      check_failure (error_flag, "unable to add group");
    }
  else
    {
      is_grp = 0;
      members = grp->gr_mem;
      // check user membership
      while (*members)
        {
          if (strcmp (*(members), username) == 0)
            {
              is_grp = 1;
              break;
            }
          members++;
        } 
      if (!is_grp) 
        {
          // add user to the group
          command_len = strlen (GROUP_NAME) + strlen (username)
                        + strlen ("usermod -a -G  ") + 1;
          command = malloc (command_len * sizeof (char));
          error_flag = snprintf (command, command_len * sizeof (char),
                                "usermod -a -G %s %s", GROUP_NAME, username);
          check_failure (error_flag, "unable to fetch group commands");
          error_flag = system (command);
          check_failure (error_flag, "unable to add user to the group");
        }
    }
  // generate the rules
  cd = open (RULES_DIR, O_WRONLY | O_CREAT | O_TRUNC,
             S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  check_failure (cd, "unable to open rules file");
  
  rules = malloc (4 * sizeof (char *));
  command_len = strlen (GROUP_NAME) + strlen ("/bin/chgrp ") + 1;
  if (command == NULL)
    {
      command = malloc (command_len * sizeof (char));
    }
  else
    {
      command = realloc (command, command_len * sizeof (char));
    }
  error_flag = snprintf (command, command_len * sizeof (char), "/bin/chgrp %s",
                         GROUP_NAME);
  check_failure (error_flag, "unable to fetch rules");
  rules[0] = generate_rule (command, screen);
  rules[1] = generate_rule (command, keyboard);
  rules[2] = generate_rule ("/bin/chmod g+w", screen);
  rules[3] = generate_rule ("/bin/chmod g+w", keyboard);

  for (int i = 0; i < 4; i++)
    {
      write (cd, rules[i], strlen (rules[i]) * sizeof (char));
    }

  error_flag = close (cd);
  check_failure (error_flag, "unable to write the rules");

  error_flag = system ("udevadm control -R");
  check_failure (error_flag, "unable to reload rules");
  command_len = strlen (SUBSYSTEM_NAME) + strlen ("udevadm trigger -c add -s ")
                + 1;
  command = realloc (command, command_len * sizeof (char));
  error_flag = snprintf (command, command_len * sizeof (char),
                         "udevadm trigger -c add -s %s", SUBSYSTEM_NAME);
  check_failure (error_flag, "unable to fetch trigger command");
  error_flag = system (command);
  check_failure (error_flag, "unable to trigger new rules");
  printf("Setup completed. You may need to logout/login or reboot.\n");
  
  free (command);
  command = NULL;
  for (int i = 0; i < 4; i++)
    {
      free (rules[i]);
    }
  free (rules);
  rules = NULL;
}

/* Generate a udev rule with SUBSYSTEM==SUBSYTEM_NAME, ACTION==ACTION_NAME.
   Target file path is selected specifing a controller and is based on
   controllers configurations.  */
static char *
generate_rule (const char *command, const enum controller_type type)
{
  int error_flag;
  char *rule;
  char *file;
  size_t file_len;
  size_t rule_len;

  file_len = strlen (controllers[type].dir) + strlen (controllers[type].name)
             + strlen ("//brightness") + 1;
  file = malloc (file_len * sizeof (char));
  error_flag = snprintf (file, file_len * sizeof (char), "%s/%s/brightness",
                         controllers[type].dir, controllers[type].name);
  check_failure (error_flag, "unable to fetch match key RUN");
  
  rule_len = strlen (SUBSYSTEM_NAME) + strlen (ACTION_NAME) + strlen (command)
             + strlen (file)
             + strlen ("SUBSYSTEM==\"\", ACTION==\"\", RUN+=\" \"\n") + 1;
  rule = malloc (rule_len * sizeof (char));
  error_flag = snprintf (rule, rule_len * sizeof (char),
                         "SUBSYSTEM==\"%s\", ACTION==\"%s\", RUN+=\"%s %s\"\n",
                         SUBSYSTEM_NAME, ACTION_NAME, command, file);
  check_failure (error_flag, "unable to fetch rule");
  
  free (file);
  return rule;
}

/* Raise an error and exit.  */
static void
throw_error (const char *message, const enum exit_status code)
{
  if (code == success)
    {
      fprintf (stderr, "%s: is this an error?\n", PROGRAM_NAME);
      exit_status = this_is_embarassing;
    }
  else
    {
      fprintf (stderr, "%s: %s\n", PROGRAM_NAME, message);
      exit_status = code;
      if (code == misuse)
       {
         usage ();
       }
    }
  exit (exit_status);
}

/* Check a result and throw an error if negative.  */
static void
check_failure (const int result, const char *message)
{
  if (result < 0)
    {
      throw_error(message, failure);
    }
}

/* Print usage.  */
static void
usage ()
{
  if (exit_status != success)
    {
      fprintf (stderr, "Try '%s --help' for more information.\n",
               PROGRAM_NAME);
    }
  else
    {
      printf ("Usage: %s [DEVICE] [OPTION]\n", PROGRAM_NAME);
      printf ("Nit is a backlight manager for screen and keyboard.\n\n\
Option:\n\
  -h, --help             display this help and exit\n\
  -l, --list             list controllers names\n\
      --setup            need root permission; generate rules to control\n\
                         brightness; this option will be executed first; see\n\
                         PERMISSIONS for more details\n\
  -s [VAL], --set=[VAL]  adjust brightness according to VAL; VAL is an\n\
                         integer and can be formatted in three ways, each of\n\
                         them is meaningfull:
                            VAL   set VAL as current brightness\n\
                           +VAL   add VAL to current brightness\n\
                           -VAL   sub VAL from current brightness\n\
  -S, --silent-mode      don't print feedback brightness value after '-s'\n\
  -v, --version          output version information and exit\n\
Device:\n\
  --screen               select screen controller\n\
  --keyboard             select keyboard controller\n\n\
Controller:\n\
By deafult controllers are 'nv_backlight' for the screen and\n\
'smc::kbd_backlight' for the keyboard. Other controllers can be used setting\n\
$NIT_CTRL_SCREEN and $NIT_CTRL_KEYBOARD. Default paths of the controllers\n\
are '/sys/class/backlight' and '/sys/class/leds'. Without '-s' option, current\n\
device's brightness is returned. In case are specified more controllers only\n\
the last will be considered.\n\n\
Permissions:\n\
In order to execute this command without root permission, you may need to add\n\
rules in '/etc/udev/rules.d'. Once you configured the controllers, you can\n\
generate these rules automatically with --setup and sudo permission. May be\n\
necessary a logout/login or a reboot.\n\n\
Note:\n\
To prevent unexpected issues brightness can never exceed minium value of 0\n\
and maximum value stored in the file `max_brightness` of the controller\n\
context.\n\n\
Exit status:\n\
  0  if OK\n\
  1  if generic failure\n\
  2  if command misuse)\n");
    }
  exit (exit_status);
}

/* Print version informations.  */
static void
version ()
{
  printf ("%s %s\n", PROGRAM_NAME, VERSION);
  printf ("Copyright (c) 2017 %s\n", AUTHOR_NAME);
  printf ("License GPLv3: GNU GPL version 3 or later \
<http://gnu.org/licenses/gpl.html>.\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n");
  printf ("Written by %s.\n", AUTHOR_NAME);
  exit (exit_status);
}
