/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/


#ifndef TCP_SERVICE_H__
#define TCP_SERVICE_H__

#include <sys/epoll.h>
#include <thread>
#include <vector>
#include <set>

#include <common.h>
#include <pipe.h>

class TCPServer;
class TCPSession;

enum TCPServiceType {
  TCP_SERVICE_TYPE_LISTEN,
  TCP_SERVICE_TYPE_NORMAL
};

class TCPService 
  : public std::enable_shared_from_this<TCPService>{
public:
  TCPService();
  ~TCPService();
  int Init(TCPServiceType service_type, int nevents,
    std::shared_ptr<TCPServer> server);

  void Start(int loop_waite_second);

  void Stop();

  int OnStartSession(TCPSession* session);

  int OnStopSession(TCPSession* session);

  // event activate
  void EventActivate();

  // consume recycle queue
  bool ConsumeRecycleQueue(PipeMsg* res_msg);

  int PushSessions(TCPSession* session);

private:
  void DoStop();

  int HandleEvent();

  void EventLoop();

  bool EventManipulate(int sockfd, int cmd, int events, void* ptr);

  void DoCheckAlive();

  TCPServiceType service_type_;
  int epoll_socket_;
  int nevents_;
  int loop_waite_second_;
  bool stopped_;
  std::vector<epoll_event> event_list_;
  // the event thread
  std::shared_ptr<std::thread> thread_;
  // the server
  std::shared_ptr<TCPServer> server_;
  // the session
  std::set<TCPSession*> sessions_;
  // the connect pipe
  std::shared_ptr<Pipe> event_push_pipe_;
  std::shared_ptr<Pipe> event_pop_pipe_;
  // the event session 
  std::shared_ptr<TCPSession> event_writer_;
  std::shared_ptr<TCPSession> event_reader_;
  DISALLOW_CONSTRUCTORS(TCPService);
};

#endif //TCP_SERVICE_H__
