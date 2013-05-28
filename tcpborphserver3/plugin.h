#ifndef PLUGIN_H_
#define PLUGIN_H_

/* Plugin command struct */
struct PLUGIN_CMD {
  char *name;
  char *desc;
  int (* cmd)(struct katcp_dispatch *d, int argc);
};

/* Plugin information struct*/
struct PLUGIN {
  int n_cmds;
  char *name;
  char *version;
  int (* init)(struct katcp_dispatch *d, int argc);
  int (* uninit)(struct katcp_dispatch *d, int argc);
  struct PLUGIN_CMD cmd_array[];
};

int list_plugin_cmd(struct katcp_dispatch *d, int argc);
int load_plugin_cmd(struct katcp_dispatch *d, int argc);
int unload_plugin_cmd(struct katcp_dispatch *d, int argc);

#endif
