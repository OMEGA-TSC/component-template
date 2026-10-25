#include "template/cmd_cli.h"
#include "template/client_com.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <console/cmddef.h>
#include "packets.h"
// SALLY backends - set during build
extern service_com_t COM_BACKEND;
service_com_t* com = &COM_BACKEND;

// communnication port handle
service_com_handle_t hcom = NULL;

// forward declarations of the commands that are to be registered
static int cli_ping(console_ctx_t *ctx, cmd_signature_t *reg);
static int cli_echo(console_ctx_t *ctx, cmd_signature_t *reg);
static int cli_kill(console_ctx_t *ctx, cmd_signature_t *reg);
static int cli_port(console_ctx_t *ctx, cmd_signature_t *reg);
static int cli_add(console_ctx_t *ctx, cmd_signature_t *reg);
static int cli_decode(console_ctx_t *ctx, cmd_signature_t *reg);
// the interface for shared libraries
#if defined(SHARED_BUILD)
void register_cli_cmds(void) {
  register_cli_example_cmds();
}

void get_version(char* semver, char* gitrev) {
  if(semver) {
    // at most 15 characters and NULL
    strncpy(semver, VERSION, 16);
  }
  if(gitrev) {
    // at most 13 characters and NULL
    strncpy(gitrev, GITREV, 14);
  }
}
#endif

// the registration function
void register_cli_example_cmds(void) {
  console_group_add("cli", "Client example");

  console_cmd_register(cli_ping, "cli ping");
  console_cmd_register(cli_echo, "cli echo");
  console_cmd_register(cli_kill, "cli kill");
  console_cmd_register(cli_port, "cli port");
  console_cmd_register(cli_add, "cli add");
  console_cmd_register(cli_decode, "cli decode");
}

// implementations of all commands follow

static int cli_ping(console_ctx_t *ctx, cmd_signature_t *reg) {
  EMPTY_CMD_SETUP("Ping the server")

  if(!hcom) {
    console_printf("No port set!\n");
    return CONSOLE_ERR_NOT_POSSIBLE;
  }
  
  char ping_buff[16] = { 0 };
  int16_t ret = client_ping(hcom, ping_buff);
  if(ret != 0) {
    console_printf("Failed, code %d\n", ret);
    return CONSOLE_ERR_IO;
  }

  console_printf("Server replied with: %s\n", ping_buff);
  return CONSOLE_OK;
}

static int cli_echo(console_ctx_t *ctx, cmd_signature_t *reg) {
  static struct {
    struct arg_str *msg;
    struct arg_end *end;
  } args;

  if(reg) {
    args.msg = arg_str1(NULL, NULL, "<msg>", EXPENDABLE_STRING("Message to echo"));
    args.end = arg_end(1);

    reg->argtable = &args;
    reg->help = EXPENDABLE_STRING("Echo a message");
    return 0;
  }

  if(!hcom) {
    console_printf("No port set!\n");
    return CONSOLE_ERR_NOT_POSSIBLE;
  }
  
  char echo_in_buff[16] = { 0 };
  int16_t ret = client_echo(hcom, echo_in_buff, (char*)args.msg->sval[0]);
  if(ret != 0) {
    console_printf("Failed, code %d\n", ret);
    return CONSOLE_ERR_IO;
  }

  console_printf("Got echo: %s\n", echo_in_buff);
  return CONSOLE_OK;
}

static int cli_kill(console_ctx_t *ctx, cmd_signature_t *reg) {
  EMPTY_CMD_SETUP("Kill the server")

  if(!hcom) {
    console_printf("No port set!\n");
    return CONSOLE_ERR_NOT_POSSIBLE;
  }
  
  int16_t ret = client_kill_server(hcom);
  if(ret != 0) {
    console_printf("Failed, code %d\n", ret);
    return CONSOLE_ERR_IO;
  }

  console_printf("Server terminated\n");
  return CONSOLE_OK;
}

static int cli_port(console_ctx_t *ctx, cmd_signature_t *reg) {
  static struct {
    struct arg_str *port;
    struct arg_end *end;
  } args;

  if(reg) {
    args.port = arg_str1(NULL, NULL, "<port>", EXPENDABLE_STRING("Port to use for communication with the server"));
    args.end = arg_end(1);

    reg->argtable = &args;
    reg->help = EXPENDABLE_STRING("Set the communication port");
    return 0;
  }

  if(!hcom) {
    hcom = com->com_handle_malloc();
  }

  // the example client does not use filesystem nor OS backends
  client_set_backends(NULL, NULL, com);

  service_com_conf_t conf = { .speed = 0 };
  com->open(hcom, (service_com_device_t)args.port->sval[0], &conf);
  return CONSOLE_OK;
}

static int cli_add(console_ctx_t *ctx, cmd_signature_t *reg) {
  static struct {
    struct arg_int *a;
    struct arg_int *b;
    struct arg_end *end;
  } args;

  if(reg) {
    args.a   = arg_int1(NULL, NULL, "<a>", EXPENDABLE_STRING("First number"));
    args.b   = arg_int1(NULL, NULL, "<b>", EXPENDABLE_STRING("Second number"));
    args.end = arg_end(2);

    reg->argtable = &args;
    reg->help = EXPENDABLE_STRING("Lest the server add two numbers");
    return 0;
  }

  if(!hcom) {
    console_printf("No port set!\n");
    return CONSOLE_ERR_NOT_POSSIBLE;
  }

  int result = 0;
  int16_t ret = client_add(hcom, &result, args.a->ival[0], args.b->ival[0]);
  if(ret != 0) {
    console_printf("Failed, code %d\n", ret);
    return CONSOLE_ERR_IO;
  }

  console_printf("Server: %d + %d = %d\n",
                 args.a->ival[0], args.b->ival[0], result);
  return CONSOLE_OK;
}

static int cli_decode(console_ctx_t *ctx, cmd_signature_t *reg) {
  static struct {
    struct arg_str *hex;
    struct arg_end *end;
  } args;

  if(reg) {
    args.hex = arg_str1(NULL, NULL, "<hex>", EXPENDABLE_STRING("32-bit instruction in Hex (for example 0x00100093)"));
    args.end = arg_end(1);
    reg->argtable = &args;
    reg->help = EXPENDABLE_STRING("Decodes a RISC-V instruction");
    return 0;
  }

  if(!hcom) {
    console_printf("No port set!\n");
    return CONSOLE_ERR_NOT_POSSIBLE;
  }

  uint32_t instr = (uint32_t)strtoul(args.hex->sval[0], NULL, 0);
  char out[DRIVER_TEMPLATE_DECODE_RSP_LEN + 1] = { 0 };
  int16_t ret = client_decode(hcom, out, instr);
  if(ret != 0) {
    console_printf("Failed, code %d\n", ret);
    return CONSOLE_ERR_IO;
  }

  console_printf("0x%08X: %s\n", (unsigned)instr, out);
  return CONSOLE_OK;
}