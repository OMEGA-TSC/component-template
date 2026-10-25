// app headers
#include "template/client_com.h"

// standard libraries
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// SALLY libraries
#include <service/service.h>
#include <service/service_filesystem.h>
#include <service/service_com.h>

// headers shared for both client and server
#include "packets.h"

// SALLY backends provided by the library user
// usually not all of those are necessary for the client
static service_fs_t* client_fs = NULL;
static service_os_t* client_os = NULL;
static service_com_t* client_com = NULL;

// forward declaration of low-level send/receive functions
static void client_cmd_send(service_com_handle_t hcom, uint8_t cmd_id, uint8_t* data, uint8_t data_len);
static size_t client_rpl_get(service_com_handle_t hcom, uint8_t* buff, size_t target_len);
int16_t client_add(service_com_handle_t hcom, int* result, int a, int b);
int16_t client_decode(service_com_handle_t hcom, char* out, uint32_t instr);

// implementation of the actual client-side C API

int16_t client_ping(service_com_handle_t hcom, char* rpl) {
  // send a ping
  client_cmd_send(hcom, DRIVER_TEMPLATE_CMD_PING, NULL, 0);

  // get the reply
  size_t ret = client_rpl_get(hcom, (uint8_t*)rpl, strlen(DRIVER_TEMPLATE_CMD_PING_RPL));
  if(ret != strlen(DRIVER_TEMPLATE_CMD_PING_RPL)) {
    return(-1);
  }

  return(0);
}

int16_t client_echo(service_com_handle_t hcom, char* out, char* in) {
  // send the echo request
  size_t echo_len = strlen(out);
  client_cmd_send(hcom, DRIVER_TEMPLATE_CMD_ECHO, (uint8_t*)out, echo_len);

  // get the reply
  size_t ret = client_rpl_get(hcom, (uint8_t*)in, echo_len);
  if(ret != echo_len) {
    return(-1);
  }

  out[ret] = '\0';
  return(0);
}

int16_t client_kill_server(service_com_handle_t hcom) {
  // send the kill request
  client_cmd_send(hcom, DRIVER_TEMPLATE_CMD_KILL, NULL, 0);
  size_t ret = client_rpl_get(hcom, NULL, 0);

  // there should be no reply to this
  if(ret != 0) {
    return(-1);
  }

  return(0);
}

// the following command/reply functions are for illustration only!
void client_cmd_send(service_com_handle_t hcom, uint8_t cmd_id, uint8_t* data, uint8_t data_len) {
  uint8_t buff[64] = { 0 };
  buff[0] = cmd_id;

  if((data != NULL) && (data_len > 0)) {
    memcpy(&buff[1], data, data_len);
  }

  memcpy(&buff[1 + data_len], DRIVER_TEMPLATE_CMD_LINE_END, strlen(DRIVER_TEMPLATE_CMD_LINE_END));
  client_com->write(hcom, buff, 1 + data_len + strlen(DRIVER_TEMPLATE_CMD_LINE_END));
}

size_t client_rpl_get(service_com_handle_t hcom, uint8_t* buff, size_t target_len) {
  size_t len = 0;
  while(len < target_len) {
    size_t res = client_com->read(hcom, buff + len, target_len - len);
    if(res > 0) {
      len += res;
    } else if(res == 0) {
      break;              // ← timeout/EOF
    } else {
      return(res);        // ← chyba
    }
  }
  return(len);
}

void client_set_backends(service_fs_t* fs, service_os_t* os, service_com_t* com) {
  client_fs = fs;
  client_os = os;
  client_com = com;
}

int16_t client_add(service_com_handle_t hcom, int* result, int a, int b) {
  char payload[24];
  int plen = snprintf(payload, sizeof(payload), "%d %d", a, b);
  if(plen <= 0 || plen > 255) {
    return(-1);
  }
  client_cmd_send(hcom, DRIVER_TEMPLATE_CMD_ADD, (uint8_t*)payload, (uint8_t)plen);
  char rsp[12] = { 0 };
  size_t ret = client_rpl_get(hcom, (uint8_t*)rsp, 11);
  if(ret != 11) {
    return(-1);
  }

  *result = atoi(rsp);
  return(0);
}

int16_t client_decode(service_com_handle_t hcom, char* out, uint32_t instr) {
  // 4 bajty little-endian
  uint8_t payload[4] = {
    (uint8_t)(instr & 0xFF),
    (uint8_t)((instr >> 8) & 0xFF),
    (uint8_t)((instr >> 16) & 0xFF),
    (uint8_t)((instr >> 24) & 0xFF)
  };
  client_cmd_send(hcom, DRIVER_TEMPLATE_CMD_DECODE, payload, 4);

  char rsp[DRIVER_TEMPLATE_DECODE_RSP_LEN] = { 0 };
  size_t ret = client_rpl_get(hcom, (uint8_t*)rsp, DRIVER_TEMPLATE_DECODE_RSP_LEN);
  if (ret != DRIVER_TEMPLATE_DECODE_RSP_LEN) {
    return -1;
  }
  rsp[DRIVER_TEMPLATE_DECODE_RSP_LEN - 1] = '\0';
  strncpy(out, rsp, DRIVER_TEMPLATE_DECODE_RSP_LEN);
  return 0;
}
