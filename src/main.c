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
  "Install packages.", NULL
},
#if 0
{
  "upgrade", 'u', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_UPGRADE,
  "Upgrade packages", NULL
},
#endif
{
  "remove", 'd', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_REMOVE,
  "Remove packages.", NULL
},
{
  "list", 'l', POPT_ARG_NONE|POPT_BIT_SET, &command, CMD_LIST,
  "List all packages. You can add package names to the command line "
  "to limit listed packages. This command supports glob matching.", NULL
},
POPT_TABLEEND
};

/* options
 ************************************************************************/

static struct cmd_options cmd_opts = {
  .root = "/",
  .dryrun = 0,
  .verbosity = 1,
  .safe = 0,
  .no_optsyms = 0,
  .no_scripts = 0,
};

static gint verbose = 0;
static gint quiet = 0;

static struct poptOption optsOptions[] = {
{
  "root", 'r', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT, &cmd_opts.root, 0,
  "Set altrernate root directory for package operations.", "ROOT"
},
{
  "safe", 's', 0, &cmd_opts.safe, 0,
  "Play it safe. Don't replace existing files/directories on install.", NULL
},
{
  "dry-run", 'n', 0, &cmd_opts.dryrun, 0,
  "Don't modify filesystem or database. This may be useful when used along "
  "with -v option to check what exactly will given command do.", NULL
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
  "no-fast-symlinks", '\0', 0, &cmd_opts.no_optsyms, 0,
  "Spkg by default parses doinst.sh for symlink creation code and removes "
  "it from the script. This improves execution times of doinst.sh. Use "
  "this option to disable such optimizations.", NULL
},
{
  "no-scripts", '\0', 0, &cmd_opts.no_scripts, 0,
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
  struct error* err;

#ifdef __DEBUG  
  g_mem_set_vtable(glib_mem_profiler_table);
#endif

  err = e_new();
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
      "  spkg -i <packages>     [--install]\n"
//      "  spkg -u <packages>     [--upgrade]\n"
      "  spkg -vd <packages>    [--verbose --remove]\n"
      "  spkg -l kde*           [--list]\n"
//      "  spkg -vnu <packages>   [--upgrade --verbose --dry-run]\n"
      "\n"
      "Official website: http://spkg.megous.com\n"
      "Bug reports can be sent to <megous@megous.com>.\n"
    );
    goto out;
  }
  if (usage)
  {
    printf(
      "Usage: spkg [-i|-d|-l] [-r ROOT] [-n] [-s] [-q] [-v] [packages...]\n"
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

 got_command:
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
    {
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_install(arg, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    }
    break;
    case CMD_UPGRADE:
    {
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_upgrade(arg, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    }
    break;
    case CMD_REMOVE:
    {
      while ((arg = poptGetArg(optCon)) != 0 && !sig_break)
      {
        if (cmd_remove(arg, &cmd_opts, err))
        {
          e_print(err);
          e_clean(err);
          status = 2;
        }
      }
    }
    break;
    case CMD_LIST:
    {
      GSList* arglist = NULL;
      while ((arg = poptGetArg(optCon)) != 0)
        arglist = g_slist_append(arglist, g_strdup(arg));
      if (cmd_list(arglist, &cmd_opts, err))
      {
        e_print(err);
        e_clean(err);
        status = 2;
      }
      g_slist_foreach(arglist, (GFunc)g_free, 0);
      g_slist_free(arglist);
    }
    break;
  }

  db_close();

 out:
  poptFreeContext(optCon);
  e_free(err);

#ifdef __DEBUG  
  g_mem_profile();
#endif

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
}
