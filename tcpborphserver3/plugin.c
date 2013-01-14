#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <dlfcn.h>
#include <katcp.h>

#include "tcpborphserver3.h"
#include "plugin.h"


int list_plugin_cmd(struct katcp_dispatch *d, int argc)
{
  int i;
  int n_plugins;
  void *module;
  char *plugin_name;
  char *plugin_vers;

  struct PLUGIN *plugin_info;

  /* Check if we haven't loaded any plugins */
  if (LOADED_PLUGINS == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no plugins have been loaded");
    return KATCP_RESULT_FAIL;
  }

  n_plugins = sizeof(LOADED_PLUGINS) / sizeof(void *);
  for (i=0; i<n_plugins; i++) {
    module = LOADED_PLUGINS[i];

    /* Get plugin info (no error checking needed)*/
    plugin_info = dlsym(module, "KATCP_PLUGIN");
    plugin_name = plugin_info->name;
    plugin_vers = plugin_info->version;

    /* Print info on each loaded module */
    log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s(%s)", plugin_name, plugin_vers);
  }

  return extra_response_katcp(d, KATCP_RESULT_OK, "%d plugin(s) loaded", n_plugins);
}

int load_plugin_cmd(struct katcp_dispatch *d, int argc)
{
  int i;
  int result;
  int n_cmds;
  int n_plugins;
  char *name;
  char *error;
  void *module;
  char *plugin_name;
  char *plugin_vers;

  struct tbs_raw *tr;
  struct PLUGIN *plugin_info;
  struct PLUGIN_CMD *plugin_cmds;

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

  /* Load dynamic library */
  module = dlopen(name, RTLD_LAZY);
  if (!module) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "could not open shared object");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }
 
  /* Get plugin info */
  plugin_info = dlsym(module, "KATCP_PLUGIN");
  error = dlerror();
  if (error) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "could not find symbol");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }

  /* Get the plugin name */
  plugin_name = plugin_info->name;
  if (plugin_name == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "plugin name not set");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }

  /* Get the plugin version */
  plugin_vers = plugin_info->version;
  if (plugin_vers == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "plugin version not set");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }

  /* Get total commands available */
  n_cmds = plugin_info->n_cmds;
  if (!n_cmds) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "plugin says no commands available");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }

  /* Get array of functions */
  plugin_cmds = plugin_info->cmd_array;
  if (plugin_cmds == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no commands found");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }

  /* Load commands into tcpborphserver */
  for (i=0; i<n_cmds; i++) {
    struct PLUGIN_CMD cmd = plugin_cmds[i];
    log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "found command: %s", cmd.name);
    result = register_flag_mode_katcp(d, cmd.name, cmd.desc, cmd.cmd, 0, TBS_MODE_RAW);
    if (result) {
      log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "error loading: ", cmd.name);
       dlclose(module); /* make sure to close on failure */
      return KATCP_RESULT_FAIL;
    }
  }

  /* Add our module to the list of loaded plugins */
  if (LOADED_PLUGINS == NULL) {
    log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "first time loading plugin, initializing plugins list");
    LOADED_PLUGINS = (void **) malloc(sizeof(void *));
  } else {
    n_plugins = sizeof(LOADED_PLUGINS) / sizeof(void *);
    log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "total plugins loaded: ", n_plugins);
    LOADED_PLUGINS = realloc(LOADED_PLUGINS, (n_plugins+1) * sizeof(void *));
  }
  if (LOADED_PLUGINS == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "problem allocing plugins list");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }
  n_plugins = sizeof(LOADED_PLUGINS) / sizeof(void *);
  LOADED_PLUGINS[n_plugins-1] = module;

   /* All done, close things cleanly */
  //dlclose(module);

  return extra_response_katcp(d, KATCP_RESULT_OK, "%d commands loaded successfully", n_cmds);
}

int unload_plugin_cmd(struct katcp_dispatch *d, int argc)
{
  return KATCP_RESULT_FAIL;
}

