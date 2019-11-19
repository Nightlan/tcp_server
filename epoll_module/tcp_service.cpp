#include <unistd.h>
#include <iostream>

#include <tcp_service.h>
#include <tcp_session.h>
#include <tcp_server.h>
#include <common.h>
#include <pipe.h>

// The listener for the pipe events on the TCPService for conn I/O
class TCPServiceEventPipeListener : public PipeEventListener {
public:
  TCPServiceEventPipeListener(const std::shared_ptr<Pipe>& pipe,
    const std::shared_ptr<TCPService>& service)
    : pipe_(pipe), service_(service) {
  }
  virtual ~TCPServiceEventPipeListener() {}

private:
  virtual void OnReadActivated(Pipe* /*pipe*/) {
    service_->EventActivate();
  }

  virtual void OnDestroy(Pipe* /*pipe*/) {
    // delete this listener when the pipe was destroyed
    delete this;
  }
  std::shared_ptr<Pipe> pipe_;
  std::shared_ptr<TCPService> service_;
};


TCPService::TCPService()
  : stopped_(true)
  , loop_waite_second_(0)
  , epoll_socket_(0) {
  //nothing
}


TCPService::~TCPService() {
}

int TCPService::Init(TCPServiceType service_type, int nevents,
  std::shared_ptr<TCPServer> server) {
  service_type_ = service_type;
  server_ = server;
  nevents_ = nevents;
  std::shared_ptr<Pipe> pipes[2];
  CHECK_RESULT(CreatePipePair(pipes, 0, true));
  event_pop_pipe_ = pipes[0];
  event_push_pipe_ = pipes[1];
  TCPServiceEventPipeListener* event_listener =
    new TCPServiceEventPipeListener(pipes[1], shared_from_this());
  event_pop_pipe_->set_listener(event_listener);
  event_pop_pipe_->CheckRead();
  event_list_.resize(nevents_);

  epoll_socket_ = epoll_create(nevents_);
  if (epoll_socket_ == -1) {
    return EPOLL_FAIL;
  }

  int pipefds[2] = { 0, 0 };
  int rst = pipe(pipefds);
  if (rst < 0) {
    return EPOLL_FAIL;
  }
  CHECK_RESULT(MakeSocketNonBlocking(pipefds[0]));
  CHECK_RESULT(MakeSocketNonBlocking(pipefds[1]));

  int event = EPOLLET | EPOLLIN;
  event_reader_.reset(new TCPSession(pipefds[0],
    TCP_SESSION_TYPE_EVENT, event));
  event = EPOLLET | EPOLLIN;
  if (!EventManipulate(pipefds[0], EPOLL_CTL_ADD,
    event, event_reader_.get())) {
    return EPOLL_FAIL;
  }
  CHECK_RESULT(event_reader_->Start());

  event = EPOLLET | EPOLLOUT;
  event_writer_.reset(new TCPSession(pipefds[1],
    TCP_SESSION_TYPE_EVENT, event));
  if (!EventManipulate(pipefds[1], EPOLL_CTL_ADD,
    event, event_writer_.get())) {
    return EPOLL_FAIL;
  }
  CHECK_RESULT(event_writer_->Start());
  return 0;
}

void TCPService::Start(int loop_waite_second) {
  loop_waite_second_ = loop_waite_second;
  stopped_ = false;
  thread_.reset(new std::thread(std::bind(&TCPService::EventLoop,
    shared_from_this())));
}

void TCPService::Stop() {
  if (stopped_) {
    return;
  }
  // send stop signal
  event_push_pipe_->Terminate();
  // stop thread
  thread_->join();

  PipeMsg msg = nullptr;
  // waiting pop pipe thread exit
  while (true) {
    int rst = event_push_pipe_->Read(-1, &msg);
    if (rst == EPOLL_EOF) {
      break;
    }
  }
  if (epoll_socket_) {
    close(epoll_socket_);
    epoll_socket_ = -1;
  }
  event_pop_pipe_.reset();
  event_push_pipe_.reset();
  event_writer_->Stop();
  event_writer_.reset();
  event_reader_->Stop();
  event_reader_.reset();
  event_list_.clear();
  thread_.reset();
  server_.reset();
  std::set<TCPSession*>::iterator it;
  for (it = sessions_.begin(); it != sessions_.end(); ++it) {
    (*it)->Stop();
    delete (*it);
  }
  sessions_.clear();
  stopped_ = true;
}

int TCPService::OnStartSession(TCPSession* session) {
  int event = session->event();
  if (MakeSocketNonBlocking(session->socket()) != 0 ||
    !EventManipulate(session->socket(), EPOLL_CTL_ADD, event, session) ||
    session->Start() != 0) {
    return EPOLL_FAIL;
  }
  sessions_.insert(session);
  return 0;
}

int TCPService::OnStopSession(TCPSession* session) {
  session->Stop();
  sessions_.erase(session);
  if (event_pop_pipe_->Recycle(session) != 0) {
    delete session;
  }
  return 0;
}

bool TCPService::EventManipulate(int sockfd, int cmd,
  int events, void* ptr) {
  epoll_event ev;
  ev.data.ptr = ptr;
  ev.events = events;
  if (epoll_ctl(epoll_socket_, cmd, sockfd, &ev) == -1) {
    return false;
  }
  return true;
}

void TCPService::DoCheckAlive() {
  std::set<TCPSession*>::iterator it;
  int64_t current_time = GetCurrentMicroseconds();
  for (it = sessions_.begin(); it != sessions_.end(); ++it) {
    if ((current_time - (*it)->last_actived_time()) > 15000000) {
      OnStopSession(*it);
    }
  }
}

void TCPService::EventActivate() {
  uint8_t falg = 0;
  event_writer_->Send(&falg, 1);
}

bool TCPService::ConsumeRecycleQueue(PipeMsg* res_msg) {
  return event_push_pipe_->ConsumeRecycleQueue(res_msg);
}

int TCPService::PushSessions(TCPSession* session) {
  return event_push_pipe_->Write(session, false);
}


void TCPService::DoStop() {
  event_pop_pipe_->Terminate();
}

int TCPService::HandleEvent() {
  PipeMsg msg = nullptr;
  while (true) {
    int rst = event_pop_pipe_->Read(0, &msg);
    if (0 == rst) {
      TCPSession* session = reinterpret_cast<TCPSession*>(msg);
      if (OnStartSession(session) != 0) {
        OnStopSession(session);
      }
    } else if (rst == EPOLL_EOF) {
      if (service_type_ == TCP_SERVICE_TYPE_LISTEN) {
        server_->DoStop();
      } 
      DoStop();
      return rst;
    } else if (rst == EPOLL_FAIL) {
      break;
    }
  }
  return 0;
}

void TCPService::EventLoop() {
  while (!stopped_) {
    int events = epoll_wait(epoll_socket_, &event_list_[0], nevents_, loop_waite_second_);
    if (events == 0) {
      if (loop_waite_second_ != -1) {
        continue;
      }
      std::cout << "epoll_wait() returned no events without timeout." << std::endl;
    }
    for (int i = 0; i < events; i++) {
      TCPSession* session = reinterpret_cast<TCPSession*>(event_list_[i].data.ptr);
      TCPSessionType type = session->session_type();
      int events = event_list_[i].events;

      if (events & (EPOLLERR | EPOLLHUP)) {
        std::cout << "epoll_wait() error on fd: " << session->socket() << std::endl;
      }
      if ((events & EPOLLIN) == EPOLLIN) {
        if ((events & EPOLLRDHUP) == EPOLLRDHUP) {
          OnStopSession(session);
        } else {
          if (type == TCP_SESSION_TYPE_LISTEN) {
            server_->HandleAccpet();
          } else {
            session->DoReceive();
            if (type == TCP_SESSION_TYPE_EVENT) {
              if (EPOLL_EOF == HandleEvent()) {
                return;
              }
            }
          }
        }
      }
      if ((events & EPOLLOUT) == EPOLLOUT) {
        session->DoSend();
      }
    }
    if (service_type_ == TCP_SERVICE_TYPE_NORMAL) {
      DoCheckAlive();
    }
  }
}

