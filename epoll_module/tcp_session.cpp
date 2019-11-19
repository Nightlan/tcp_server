#include <tcp_session.h>
#include <tcp_service.h>
#include <message_parser.h>

#include <sys/socket.h>
#include <unistd.h>

TCPSession::TCPSession(int socket, TCPSessionType type, int event)
  : socket_(socket)
  , event_(event)
  , session_type_(type)
  , stopped_(true)
  , write_waiting_(false)
  , last_actived_time_(0)
  , write_offset_(0)
  , send_buffer_(nullptr)
  , wait_buffer_(nullptr){
  //nothing
}


TCPSession::~TCPSession() {
  //nothing
}


int TCPSession::Start() {
  message_parser_.reset(new MessageParser(this));
  send_buffer_ = &buffers_[0];
  wait_buffer_ = &buffers_[1];
  stopped_ = false;
  last_actived_time_ = GetCurrentMicroseconds();
  return 0;
}

void TCPSession::Stop() {
  if (stopped_) {
    return;
  }
  if (socket_ >= 0) {
    shutdown(socket_, SHUT_RDWR);
    close(socket_);
    socket_ = -1;
  }
  message_parser_.reset();
  write_offset_ = 0;
  send_buffer_->Clear();
  wait_buffer_->Clear();
  write_waiting_ = false;
  stopped_ = true;
}

int TCPSession::DoReceive() {
  if (socket_ < 0 || stopped_) {
    return 0;
  }
  while (true) {
    int rst = 0;
    rst = read(socket_, recv_buffer_, kRecvBufferSize);
    if (rst == 0) {
      return EPOLL_FAIL;
    } else if (rst == -1) {
      int error_code = errno;
      if (error_code != EAGAIN && 
        error_code != EINTR &&
        error_code != EWOULDBLOCK) {
        return EPOLL_FAIL;
      }
      break;
    } else {
      last_actived_time_ = GetCurrentMicroseconds();
      message_parser_->Parser(recv_buffer_, rst);
    }
  }
  return 0;
}

int TCPSession::DoSend() {
  if (socket_ < 0 || stopped_) {
    return 0;
  }
  int rst = 0;
  while ((rst = Write()) == 0) {
    WriteFlush();
  }
  if (rst == EPOLL_BUSY) {
    write_waiting_ = true;
  } else if(rst == EPOLL_NO_DATA) {
    write_waiting_ = false;
  }
  return rst;
}

int TCPSession::Send(const uint8_t* buffer, int size) {
  if (socket_ < 0 || stopped_) {
    return 0;
  }
  
  if (write_waiting_) {
    wait_buffer_->Write(buffer, size);
  } else {
    send_buffer_->Write(buffer, size);
    return DoSend();
  }
  return 0;
}


int TCPSession::Write() {
  int send_length = send_buffer_->size();
  if (send_length == 0) {
    return EPOLL_NO_DATA;
  }
  uint8_t* wirte_buffer = send_buffer_->begin();
  while (write_offset_ < send_length) {
    int rst = write(socket_, wirte_buffer + write_offset_,
      send_length - write_offset_);

    if (rst <= 0) {
      int error_code = errno;
      if (error_code != EAGAIN &&
        error_code != EINTR &&
        error_code != EWOULDBLOCK) {
        return EPOLL_FAIL;
      }
      return EPOLL_BUSY;
    } else {
      write_offset_ += rst;
    }
  }
  return 0;
}

void TCPSession::WriteFlush() {
  // switch the waiting buffer with the send buffer
  send_buffer_->Clear();
  ByteArray* tmp = send_buffer_;
  send_buffer_ = wait_buffer_;
  wait_buffer_ = tmp;
  write_offset_ = 0;
}

