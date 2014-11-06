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


int N_LOADED_PLUGINS = 0;
void **LOADED_PLUGINS = NULL;


int list_plugin_cmd(struct katcp_dispatch *d, int argc)
{
  int i;
  void *module;
  char resp_str[80];
  char *plugin_name;
  char *plugin_vers;

  struct PLUGIN *plugin_info;

  /* Check if we haven't loaded any plugins */
  if (N_LOADED_PLUGINS == 0) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no plugins have been loaded");
    return KATCP_RESULT_FAIL;
  }

  /* Log how many plugins are loaded */
  log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%d plugin(s) loaded", N_LOADED_PLUGINS);

  /* Initialize the response */
  prepend_reply_katcp(d);
  append_string_katcp(d, KATCP_FLAG_STRING, KATCP_OK);

  /* Append each name/vers to the response */
  for (i=0; i<N_LOADED_PLUGINS; i++) {
    module = LOADED_PLUGINS[i];

    /* Get plugin info (no error checking needed)*/
    plugin_info = dlsym(module, "KATCP_PLUGIN");
    plugin_name = plugin_info->name;
    plugin_vers = plugin_info->version;

    /* Append info on each loaded module */
    sprintf(resp_str, "%s(%s)", plugin_name, plugin_vers);
    append_string_katcp(d, KATCP_FLAG_STRING, resp_str);
  }

  /* Finally append total number of plugins */
  append_double_katcp(d, KATCP_FLAG_DOUBLE | KATCP_FLAG_LAST, N_LOADED_PLUGINS);
  return KATCP_RESULT_OWN;
}

int load_plugin_cmd(struct katcp_dispatch *d, int argc)
{
  int i;
  int result;
  int n_cmds;
  char *name;
  char *error;
  void *module, *i_module;
  char *plugin_name, *i_plugin_name;
  char *plugin_vers, *i_plugin_vers;

  struct PLUGIN *plugin_info;
  struct PLUGIN *i_plugin_info;
  struct PLUGIN_CMD *plugin_cmds;

  int (* plugin_init)(struct katcp_dispatch *d, int argc);

  /* Get the filename of the plugin to load */
  name = arg_string_katcp(d, 1);
  if(name == NULL){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "internal failure while acquiring parameters");
    return KATCP_RESULT_FAIL;
  }

  /* Load dynamic library */
  module = dlopen(name, RTLD_NOW | RTLD_GLOBAL | RTLD_DEEPBIND);
  if (!module) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "could not open shared object: %s", dlerror());
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

  /* Check if we've already loaded this plugin */
  for (i=0; i<N_LOADED_PLUGINS; i++) {
    i_module = LOADED_PLUGINS[i];

    /* Get plugin info (no error checking needed)*/
    i_plugin_info = dlsym(i_module, "KATCP_PLUGIN");
    i_plugin_name = i_plugin_info->name;
    i_plugin_vers = i_plugin_info->version;

    if (strcmp(plugin_name, i_plugin_name) == 0) {
      log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "%s(%s) already loaded!", i_plugin_name, i_plugin_vers);
      dlclose(module); /* make sure to close on failure */
      return KATCP_RESULT_FAIL;
    }
  }

  /* Get the init if available */
  plugin_init = plugin_info->init;
  if (plugin_init == NULL) {
    log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "no plugin init function found");
  } else {
    /* Call init before loading commands */
    if (plugin_init(d, argc) != 0) {
      log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "problem while initializing");
      dlclose(module); /* make sure to close on failure */
      return KATCP_RESULT_FAIL;
    }
  }

  /* Get total commands available */
  n_cmds = plugin_info->n_cmds;
  if (n_cmds == 0) {
    log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "plugin says no commands available");
  }

  /* Get array of functions */
  plugin_cmds = plugin_info->cmd_array;
  if (plugin_cmds == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no commands found");
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

  /* Expand the plugins list */
  void **temp = realloc(LOADED_PLUGINS, (N_LOADED_PLUGINS+1) * sizeof(void *));
  if (temp == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "problem reallocing plugins list");
    dlclose(module); /* make sure to close on failure */
    return KATCP_RESULT_FAIL;
  }

  /* Add our module to new list */
  LOADED_PLUGINS = temp;
  LOADED_PLUGINS[N_LOADED_PLUGINS] = module;

  /* And finally increment the plugins counter */
  N_LOADED_PLUGINS++;

  return extra_response_katcp(d, KATCP_RESULT_OK, "%d commands loaded successfully", n_cmds);
}

int unload_plugin_cmd(struct katcp_dispatch *d, int argc)
{
  int i, j, k;
  int past = 0;
  int found = 0;
  int result;
  int n_cmds;
  char *name;
  void *module;
  char *plugin_name;

  struct PLUGIN *plugin_info;
  struct PLUGIN_CMD *plugin_cmds;

  int (* plugin_uninit)(struct katcp_dispatch *d, int argc);

  /* Check if we haven't loaded any plugins */
  if (LOADED_PLUGINS == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no plugins have been loaded");
    return KATCP_RESULT_FAIL;
  }

  /* Get the name of the plugin to unload */
  name = arg_string_katcp(d, 1);
  if(name == NULL){
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "internal failure while acquiring parameters");
    return KATCP_RESULT_FAIL;
  }

  for (i=0; i<N_LOADED_PLUGINS; i++) {
    module = LOADED_PLUGINS[i];

    /* Get plugin info (no error checking needed)*/
    plugin_info = dlsym(module, "KATCP_PLUGIN");
    plugin_name = plugin_info->name;

    if (strcmp(name, plugin_name) == 0) {
      /* Print info on each loaded module */
      log_message_katcp(d, KATCP_LEVEL_INFO, NULL, "found plugin %s", plugin_name);
      found = 1;
      break;
    }
  }

  /* Get out if plugin hasn't been loaded */
  if (!found) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "plugin not found. cannot unload");
    return KATCP_RESULT_FAIL;
  }

  /* Get the uninit if available */
  plugin_uninit = plugin_info->uninit;
  if (plugin_uninit == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "no plugin uninit function found");
  } else {
    /* Call uninit before closing */
    if (plugin_uninit(d, argc) != 0) {
      log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "problem while uninitializing");
      return KATCP_RESULT_FAIL;
    }
  }

  /* Now deregister the katcp commands */
  n_cmds = plugin_info->n_cmds;
  plugin_cmds = plugin_info->cmd_array;
  for (j=0; j<n_cmds; j++) {
    struct PLUGIN_CMD cmd = plugin_cmds[j];
    result = deregister_command_katcp(d, cmd.name);
    if (result) {
      log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "problem deregistering: ", cmd.name);
      return KATCP_RESULT_FAIL;
    }
  }

  /* Now close the dynamic module */
  dlclose(module);

  /* Allocate new plugins list */
  void **temp = malloc((N_LOADED_PLUGINS-1) * sizeof(void *));
  if (temp == NULL) {
    log_message_katcp(d, KATCP_LEVEL_ERROR, NULL, "problem allocing plugins list");
    return KATCP_RESULT_FAIL;
  }

  /* Remove module from plugins list */
  for (k=0; k<N_LOADED_PLUGINS; k++) {
    if (k == i) {
      past = 1;
    } else {
      temp[k-past] = LOADED_PLUGINS[k];
    }
  }
  free(LOADED_PLUGINS);
  LOADED_PLUGINS = temp;

  /* And finally decrement the plugins counter */
  N_LOADED_PLUGINS--;

  return extra_response_katcp(d, KATCP_RESULT_OK, "%s plugin unloaded", name);
}

