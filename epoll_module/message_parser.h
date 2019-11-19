/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/
/// @file Contains the message parser

#ifndef EPOLL_MESSAGE_PARSER
#define EPOLL_MESSAGE_PARSER

#include <common.h>
#include <memory>

class TCPSession;
class MessageParser {
 public:
  MessageParser(TCPSession* session);
  ~MessageParser();

  int Parser(const uint8_t* data, int size);

 private:
   // message parser session
   TCPSession* session_;
   // Disable copying of MessageParser
   DISALLOW_CONSTRUCTORS(MessageParser);
};

#endif // !EPOLL_MESSAGE_PARSER



