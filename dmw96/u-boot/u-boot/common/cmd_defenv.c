#include <common.h>
#include <command.h>
#include <environment.h>

int do_defenv(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	set_default_env(NULL);
	return 0;
}

U_BOOT_CMD(
	defenv, 1, 0, do_defenv,
	"defenv\t- reset to default environment\n",
	"\n"
);

