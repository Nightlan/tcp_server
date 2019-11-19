/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/


#ifndef TCP_SESSION_H__
#define TCP_SESSION_H__

#include <common.h>
#include <byte_array.h>
#include <memory>

enum TCPSessionType {
  TCP_SESSION_TYPE_EVENT,
  TCP_SESSION_TYPE_LISTEN,
  TCP_SESSION_TYPE_NORMAL
};

class TCPService;
class MessageParser;

class TCPSession {
public:
  TCPSession(int socket, TCPSessionType type, int event);
  ~TCPSession();

  int Start();

  void Stop();

  int socket() {
    return socket_;
  }

  int event() {
    return event_;
  }

  const int64_t& last_actived_time() const {
    return last_actived_time_;
  }

  TCPSessionType session_type() const {
    return session_type_;
  }
  int DoReceive();

  int DoSend();

  int Send(const uint8_t* buffer, int size);
private:
  static const int kRecvBufferSize = 8196;

  int Write();

  void WriteFlush();

  bool stopped_;
  bool write_waiting_;
  int socket_;
  int event_;
  int64_t last_actived_time_;
  // the message parser
  std::shared_ptr<MessageParser> message_parser_;
  // the session type
  TCPSessionType session_type_;
  // the recv buffer
  uint8_t recv_buffer_[kRecvBufferSize];
  // the send buffers
  ByteArray buffers_[2];
  // the current sending buffer
  ByteArray* send_buffer_;
  // the waiting buffer
  ByteArray* wait_buffer_;
  // the write offset
  int write_offset_;
  DISALLOW_CONSTRUCTORS(TCPSession);
};

#endif //TCP_SESSION_H__

