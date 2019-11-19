
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

#include <common.h>
#include <tcp_service.h>
#include <tcp_server.h>
#include <tcp_session.h>

TCPServer::TCPServer()
  : next_service_(0)
  , stopped_(true){
  //nothing
}

TCPServer::~TCPServer() {
  //nothing
}

int TCPServer::InitServer(const char* ip_adress, int port,
                          int epoll_module_count) {
  if (epoll_module_count == 0) {
    return EPOLL_FAIL;
  }
  epoll_module_count_ = epoll_module_count;
  listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

  sockaddr_in server_sockaddr;
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(port);
  in_addr_t addr = inet_addr(ip_adress);
  if (addr == INADDR_NONE) {
    return EPOLL_FAIL;
  }
  server_sockaddr.sin_addr.s_addr = addr;
  sockaddr* addr_info = reinterpret_cast<sockaddr*>(&server_sockaddr);
  if (bind(listen_socket_, addr_info, sizeof(server_sockaddr)) == -1) {
    return EPOLL_FAIL;
  }
  return 0;
}

int TCPServer::StartServer() {
  int rst = 0;
  rst = listen(listen_socket_, SOMAXCONN);
  if (rst == -1) {
    return EPOLL_FAIL;
  }

  // start event service
  for (int i = 0; i < epoll_module_count_; ++i) {
    std::shared_ptr<TCPService> tcp_service(new TCPService());
    CHECK_RESULT(tcp_service->Init(TCP_SERVICE_TYPE_NORMAL,
      128, shared_from_this()));
    tcp_service->Start(3000);
    services_.push_back(tcp_service);
  }
  // start listen service
  listen_service_.reset(new TCPService());
  CHECK_RESULT(listen_service_->Init(TCP_SERVICE_TYPE_LISTEN, 
    128, shared_from_this()));
  listen_service_->Start(-1);
  // add listen socket
  int event = EPOLLET | EPOLLIN;
  TCPSession* listen_session = new TCPSession(listen_socket_,
    TCP_SESSION_TYPE_LISTEN, event);
  CHECK_RESULT(listen_service_->PushSessions(listen_session));
  stopped_ = false;
  return 0;
}

void TCPServer::StopServer() {
  if (stopped_) {
    return;
  }
  listen_service_->Stop();
  services_.clear();
  if (listen_socket_ > 0) {
    close(listen_socket_);
    listen_socket_ = -1;
  }
  stopped_ = true;
}

void TCPServer::DoStop() {
  for (size_t i = 0; i < services_.size(); ++i) {
    services_[i]->Stop();
  }
}

int TCPServer::HandleAccpet() {
  if (stopped_) {
    return 0;
  }
  // address info
  int conn_socket = 0;
  struct sockaddr_in conn_address;
  socklen_t addrLen = sizeof(conn_address);

  // session info
  int event = EPOLLET | EPOLLIN | EPOLLOUT| EPOLLRDHUP;
  std::shared_ptr<TCPService> service;
  TCPSession* session = nullptr;
  PipeMsg msg = nullptr;

  // get socket
  while ((conn_socket = accept(listen_socket_,
    (struct sockaddr *)&conn_address, &addrLen)) > 0) {
    service = GetNextService();
    if (service->ConsumeRecycleQueue(&msg)) {
      session = new(msg) TCPSession(conn_socket,
        TCP_SESSION_TYPE_NORMAL, event);
    } else {
      session = (new TCPSession(conn_socket,
        TCP_SESSION_TYPE_NORMAL, event));
    }
    service->PushSessions(session);
  }
  if (conn_socket == -1) {
    if (errno != EAGAIN && errno != ECONNABORTED 
      && errno != EPROTO && errno != EINTR) {
      std::cout << "accept error, errno: " << errno << std::endl;
    }
  }
  return 0;
}

const std::shared_ptr<TCPService>& TCPServer::GetNextService() {
  const std::shared_ptr<TCPService>& result = services_[next_service_];
  next_service_++;
  if (next_service_ == static_cast<int>(services_.size())) {
    next_service_ = 0;
  }
  return result;
}
