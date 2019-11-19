/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/


#ifndef TCP_SERVER_H__
#define TCP_SERVER_H__

#include <vector>
#include <memory>
#include <common.h>

class TCPService;
class Pipe;

class TCPServer
  : public std::enable_shared_from_this<TCPServer> {
public:
  TCPServer();
  ~TCPServer();

  int InitServer(const char* ip_adress, int port, int epoll_module_count);

  int StartServer();

  void StopServer();

  void DoStop();

  int HandleAccpet();
private:
  // the simple load balancing
  const std::shared_ptr<TCPService>& GetNextService();

  bool stopped_;
  // the next service
  int next_service_;
  // the listen socket
  int listen_socket_;
  // the epoll module count
  int epoll_module_count_;
  // the service vector
  std::vector<std::shared_ptr<TCPService>> services_;
  // the listen service
  std::shared_ptr<TCPService> listen_service_;
  DISALLOW_CONSTRUCTORS(TCPServer);
};

#endif // ! TCP_SERVER_H__


