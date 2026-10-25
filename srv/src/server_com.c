// app headers
#include "template/server_com.h"
#include "template/rv32im_decoder.h"
// standard libraries
#include <string.h>
#include <stdio.h>

// SALLY libraries
#include <service/service.h>
#include <service/service_filesystem.h>
#include <service/service_com.h>

// headers shared for both client and server
#include "packets.h"

// SALLY backends provided by the library user
static service_fs_t* server_fs = NULL;
static service_os_t* server_os = NULL;
static service_com_t* server_com = NULL;

void server_set_backends(service_fs_t* fs, service_os_t* os, service_com_t* com) {
  server_fs = fs;
  server_os = os;
  server_com = com;
}

int server_process_cmd(service_com_handle_t hcom, uint8_t* buff, size_t len) {
  // an example of a very simple processing
  switch(buff[0]) {
    case DRIVER_TEMPLATE_CMD_PING: {
      // on ping, just send the defined reply
      const char* rpl = DRIVER_TEMPLATE_CMD_PING_RPL;
      server_com->write(hcom, (void*)rpl, strlen(rpl));
    } break;

    case DRIVER_TEMPLATE_CMD_ECHO: {
      // on echo, send everything back, except the command byte
      uint8_t* pos = (uint8_t*)strstr((char*)buff, DRIVER_TEMPLATE_CMD_LINE_END);
      if(pos != NULL) {
        size_t echo_len = pos-buff-1;
        server_com->write(hcom, (void*)&buff[1], echo_len);
        server_com->write(hcom, (void*)DRIVER_TEMPLATE_CMD_LINE_END, strlen(DRIVER_TEMPLATE_CMD_LINE_END));
      }
    } break;
    case DRIVER_TEMPLATE_CMD_ADD: {
      uint8_t* pos = (uint8_t*)strstr((char*)buff, DRIVER_TEMPLATE_CMD_LINE_END);
      if(pos != NULL) {
        int a = 0, b = 0;
        if(sscanf((char*)&buff[1], "%d %d", &a, &b) == 2) {
          char rsp[12];
          snprintf(rsp, sizeof(rsp), "%011d", a + b);
          server_com->write(hcom, rsp, 11);
        }
      }
    } break;
    case DRIVER_TEMPLATE_CMD_DECODE: {
  if (len >= 7 && buff[5] == '\r' && buff[6] == '\n') {
    uint32_t instr = (uint32_t)buff[1]
                  | ((uint32_t)buff[2] << 8)
                  | ((uint32_t)buff[3] << 16)
                  | ((uint32_t)buff[4] << 24);
    char rsp[DRIVER_TEMPLATE_DECODE_RSP_LEN] = { 0 };
    rv32im_decode(instr, rsp, sizeof(rsp));
    server_com->write(hcom, rsp, DRIVER_TEMPLATE_DECODE_RSP_LEN);
  }
} break;

    case DRIVER_TEMPLATE_CMD_KILL:
      // on kill, stop processing 
      return(DRIVER_TEMPLATE_CMD_KILL);

    default:
      break;
  }

  return(0);
}
