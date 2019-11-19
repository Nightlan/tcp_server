#include <message_parser.h>
#include <tcp_session.h>
#include <iostream>
MessageParser::MessageParser(TCPSession* session)
  : session_(session){
}

MessageParser::~MessageParser() {
}

int MessageParser::Parser(const uint8_t* data, int size) {
  if (session_->session_type() == TCP_SESSION_TYPE_EVENT) {
    std::cout << "new session" << std::endl;
  } else if (session_->session_type() == TCP_SESSION_TYPE_NORMAL) {
    std::cout << "receiveed: " << size << " byte." << std::endl;
  }
  return 0;
}