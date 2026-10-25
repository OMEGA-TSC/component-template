#ifndef DRIVER_TEMPLATE_PACKETS_H
#define DRIVER_TEMPLATE_PACKETS_H

/*! 
  \file packets.h
  \brief This file defines the communication interface between client and server.
  Usually, this is an implementation of the device ICD.
  
  You can either define the protocol/packets manually here, or use generate them
  using lpldgen in common/lpldgen. The latter is usually preferrable,
  as lplgen will also generate all of the parsing/building functions for you.
*/

/*! 
  \defgroup driver_packets Template packet values
  @{
*/
#define DRIVER_TEMPLATE_CMD_LINE_END          "\r\n"
#define DRIVER_TEMPLATE_CMD_PING              (0x00)
#define DRIVER_TEMPLATE_CMD_PING_RPL          "Hello World!" DRIVER_TEMPLATE_CMD_LINE_END
#define DRIVER_TEMPLATE_CMD_ECHO              (0x01)
#define DRIVER_TEMPLATE_CMD_ECHO_MAX_LEN      (64)
#define DRIVER_TEMPLATE_CMD_KILL              (0x02)
#define DRIVER_TEMPLATE_CMD_ADD               (0x04)
#define DRIVER_TEMPLATE_CMD_DECODE            (0x05)
#define DRIVER_TEMPLATE_DECODE_RSP_LEN        (64)
/*!
  @}
*/

#endif
