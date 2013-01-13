#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <katcp.h>

#include "tcpborphserver3.h"
#include "plugin.h"

int load_plugin_cmd(struct katcp_dispatch *d, int argc)
{
  char *name;
  struct tbs_raw *tr;

  tr = get_current_mode_katcp(d);
  if(tr == NULL){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "unable to get raw state");
    return KATCP_RESULT_FAIL;
  }

  name = arg_string_katcp(d, 1);
  if(name == NULL){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "internal failure while acquiring parameters");
    return KATCP_RESULT_FAIL;
  }

  extra_response_katcp(d, KATCP_RESULT_OK, name);
  return KATCP_RESULT_OWN;
}
