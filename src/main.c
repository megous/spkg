/*----------------------------------------------------------------------*\
|* spkg - The Unofficial Slackware Linux Package Manager                *|
|*                                      designed by Ond�ej Jirman, 2005 *|
|*----------------------------------------------------------------------*|
|*          No copy/usage restrictions are imposed on anybody.          *|
\*----------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <popt.h>

#include "commands.h"
#include "pkgdb.h"
#include "sigtrap.h"

/* commands
 ************************************************************************/

static guint command = 0;
#define CMD_INSTALL (1<<0)
#define CMD_UPGRADE (1<<1)
#define CMD_REMOVE  (1<<2)
#define CMD_LIST    (1<<3)

static struct poptOption optsCommands[] = {
{
  "install", 'i', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_INSTALL, 
  "Install packages. ([p]aranoid|<n>ormal|[b]rutal)", NULL
},
#if 0
{
  "upgrade", 'u', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_UPGRADE,
  "Upgrade packages", NULL
},
{
  "remove", 'd', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_REMOVE,
  "Remove packages", NULL
},
#endif
{
  "list", 'l', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_LIST,
  "List packages. (<a>ll|[g]lob)", NULL
},
POPT_TABLEEND
};

/* commands/modes definition
 ************************************************************************/

struct mode {
  gint mode;
  gchar* shortcut;
  gchar* longname;
};
struct cmd {
  gint cmd;
  gint default_mode;
  struct mode modes[16];
};
static struct cmd cmds[] = {
{
  CMD_INSTALL, CMD_MODE_NORMAL, {
    { CMD_MODE_PARANOID, "p", "paranoid" },
    { CMD_MODE_NORMAL, "n", "normal" },
    { CMD_MODE_BRUTAL, "b", "brutal" },
    { 0 },
  }
},
{
  CMD_LIST, CMD_MODE_ALL, {
    { CMD_MODE_ALL, "a", "all" },
    { CMD_MODE_GLOB, "g", "glob" },
    { 0 },
  }
},
{ 0 }
};

/* options
 ************************************************************************/

static struct cmd_options cmd_opts = {
  .root = "/",
  .dryrun = 0,
  .verbosity = 1,
};

static gchar* mode = 0;
static gint verbose = 0;
static gint quiet = 0;
static gint no_optsyms = 0;
static gint no_scripts = 0;

static struct poptOption optsOptions[] = {
{
  "mode", 'm', POPT_ARG_STRING, &mode, 0,
  "Set command mode of operation. See particular command for available "
  "modes.", "MODE"
},
{
  "root", 'r', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &cmd_opts.root, 0,
  "Set altrernate root directory for package operations.", "ROOT"
},
{
  "verbose", 'v', 0, 0, 1,
  "Increase verbosity level. You may use this option multiple times to get "
  "even more verbose output.", NULL
},
{
  "quiet", 'q', 0, &quiet, 0,
  "Set verbosity level to zero. Default verbosity level is 1 (show only "
  "warnings). This option disables warnings. Please note, that error "
  "messages can't be disabled.", NULL
},
{
  "dry-run", 'n', 0, &cmd_opts.dryrun, 0,
  "Don't modify filesystem or database. This may be useful when used along "
  "with -v option to check what exactly will given command do.", NULL
},
{
  "no-fast-symlinks", '\0', 0, &no_optsyms, 0,
  "Spkg by default parses doinst.sh for symlink creation code and removes "
  "it from the script. This improves execution times of doinst.sh. Use "
  "this option to disable such optimizations.", NULL
},
{
  "no-scripts", '\0', 0, &no_scripts, 0,
  "Disable postinstallation script.", NULL
},
POPT_TABLEEND
};

/* help
 ************************************************************************/

static gint help = 0;
static gint usage = 0;
static gint version = 0;

static struct poptOption optsHelp[] = {
{
  "usage", '\0', POPT_ARG_NONE, &usage, 0,
  "Display brief usage message.", NULL
},
{
  "help", 'h', POPT_ARG_NONE, &help, 0,
  "Show this help message.", NULL
},
{
  "version", 'V', POPT_ARG_NONE, &version, 0,
  "Display spkg version.", NULL
},
POPT_TABLEEND
};

/* main table
 ************************************************************************/

static struct poptOption opts[] = {
{ NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsCommands, 0, "Commands:", NULL },
{ NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsOptions, 0, "Options:", NULL },
{ NULL, '\0', POPT_ARG_INCLUDE_TABLE, &optsHelp, 0, "Help options:", NULL },
POPT_TABLEEND
};

/* main
 ************************************************************************/

int main(const int ac, const char* av[])
{
  poptContext optCon=0;
  gint rc;
  gint status = 0;
  const gchar* arg;
  struct error* err = e_new();

  /* check if we have enough privileges */
  unsetenv("LD_LIBRARY_PATH");
  if (getuid() != 0)
  {
    fprintf(stderr, "You need root privileges to run this program. Sorry.\n");
    exit(1);
  }

  /* initialize popt context */
  optCon = poptGetContext("spkg", ac, av, opts, 0);
  poptSetOtherOptionHelp(optCon, "<command> [options] [packages...]");

  /* parse options */
  while ((rc = poptGetNextOpt(optCon)) != -1)
  {
    if (rc == 1)
      verbose++;
    if (rc < -1)
    {
      fprintf(stderr, "error[main]: invalid argument: %s (%s)\n",
        poptStrerror(rc),
        poptBadOption(optCon, POPT_BADOPTION_NOALIAS));
      goto err_1;
    }
  }

  /* these are help handlers */
  if (help)
  {
    printf(
      "spkg " G_STRINGIFY(SPKG_VERSION) "\n"
      "\n"
      "Written by Ondrej Jirman, 2005\n"
      "\n"
      "This is free software. Not like a beer or like in a \"freedom\",\n"
      "but like in \"I don't care what you are going to do with it.\"\n"
      "\n"
    );
    poptPrintHelp(optCon, stdout, 0);
    printf(
      "\n"
      "Examples:\n"
      "  spkg -imb <packages>   [--install --mode=brutal]\n"
      "  spkg -ump <packages>   [--upgrade --mode=paranoid]\n"
      "  spkg -vr <packages>    [--verbose --remove --mode=normal]\n"
      "  spkg -vnumb <packages> [--upgrade --verbose --dry-run --mode=brutal]\n"
      "\n"
      "Official website: http://spkg.megous.com\n"
      "Bug reports can be sent to <megous@megous.com>.\n"
    );
    goto out;
  }
  if (usage)
  {
    printf(
      "Usage: spkg [-i|--install] [-m|--mode MODE] [-r|--root ROOT]\n"
      "            [-n|--dry-run] [-q|--quiet] [-v|--verbose] [packages...]\n"
    );
    goto out;
  }
  if (version)
  {
    printf("spkg-" G_STRINGIFY(SPKG_VERSION) "\n");
    goto out;
  }

  /* got command? */
  if (command == 0)
  {
    /* check if we are run as Xpkg */
    if (av[0] != 0)
    {
      gchar* b = g_path_get_basename(av[0]);
      if (!strcmp(b, "ipkg"))
        command |= CMD_INSTALL;
      else if (!strcmp(b, "rpkg"))
        command |= CMD_REMOVE;
      else if (!strcmp(b, "upkg"))
        command |= CMD_UPGRADE;
      else if (!strcmp(b, "lpkg"))
        command |= CMD_LIST;
      g_free(b);
      if (command)
        goto got_command;
    }
    fprintf(stderr, "error[main]: invalid argument: no command given\n");
    goto err_1;
  }
 got_command:;

  /* get mode for command */
  struct cmd* c = cmds;
  gint cmd_mode;
  while (c->cmd) /* for each command */
  {
    if (c->cmd == command)
    {
      /* command found */
      cmd_mode = c->default_mode;
      if (mode == 0) /* no mode specified on command line */
        goto mode_ok;
      struct mode* m = c->modes;
      while (m->shortcut) /* for each mode */
      {
        if (!strcmp(m->shortcut, mode) || !strcmp(m->longname, mode))
        {
          cmd_mode = m->mode;
          goto mode_ok;
        }
        m++;
      }
      goto no_mode;
    }
    c++;
  }
  /* command not found in a table (because it is incomplete!) */
  g_assert_not_reached();
 no_mode:
  /* mode not found in a table */
  fprintf(stderr, "error[main]: invalid argument: unknown mode (%s)\n", mode);
  goto err_1;
 mode_ok:

  /* check verbosity options */
  if (verbose && quiet)
  {
    fprintf(stderr, "error[main]: invalid argument: verbose or quiet?\n");
    goto err_1;
  }
  if (verbose)
    cmd_opts.verbosity = verbose+1;
  if (quiet)
    cmd_opts.verbosity = 0;

  /* check command options */
  switch (command)
  {
    case CMD_INSTALL:
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
    break;
    case CMD_UPGRADE:
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
    break;
    case CMD_REMOVE:
      if (poptPeekArg(optCon) == 0)
        goto err_nopackages;
    break;
    case CMD_LIST:
      if (cmd_mode == CMD_MODE_ALL)
      {
        if (poptPeekArg(optCon) != 0)
          goto err_garbage;
      }
      else
      {
        if (poptPeekArg(optCon) == 0)
          goto err_nopackages;
      }
    break;
    default:
      fprintf(stderr, "error[main]: invalid argument: schizofrenic command usage\n");
      goto err_1;
  }

  /* init signal trap */
  if (sig_trap(err))
    goto err_2;

  /* open db */
  if (db_open(cmd_opts.root, err))
    goto err_2;

  switch (command)
  {
    case CMD_INSTALL:
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_install(arg, cmd_mode, no_optsyms, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    break;
    case CMD_UPGRADE:
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_upgrade(arg, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    break;
    case CMD_REMOVE:
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_remove(arg, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    break;
    case CMD_LIST:
      if (cmd_mode != CMD_MODE_ALL)
      {
        while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
        {
          if (cmd_list(arg, cmd_mode, &cmd_opts, err))
          {
            e_print(err);
            e_clean(err);
            status = 2;
          }
        }
      }
      else
      {
        if (cmd_list(0, cmd_mode, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    break;
  }

  db_close();

 out:
  poptFreeContext(optCon);
  e_free(err);
  /* 0 = all ok
   * 1 = command line error
   * 2 = package manager error
   */
  return status;
 err_1:
  status = 1;
  goto out;
 err_2:
  status = 2;
  e_print(err);
  goto out;
 err_nopackages:
  fprintf(stderr, "error[main]: invalid argument: no packages given\n");
  goto err_1;
 err_garbage:
  fprintf(stderr, "error[main]: invalid argument: garbage on command line (%s...)\n", poptPeekArg(optCon));
  goto err_1;
}
