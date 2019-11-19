/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/

#include <pipe.h>

int CreatePipePair(std::shared_ptr<Pipe> pipes[2], int index, bool enable_recycle) {
  std::shared_ptr<Pipe> pipe0(new (std::nothrow) Pipe(index, enable_recycle));
  std::shared_ptr<Pipe> pipe1(new (std::nothrow) Pipe(index, enable_recycle));
  
  Pipe::MessageQueue* queues[2] = {0};
  queues[0] = new (std::nothrow) Pipe::MessageQueue();
  queues[1] = new (std::nothrow) Pipe::MessageQueue();
  CHECK_RESULT(queues[0]->Init());
  CHECK_RESULT(queues[1]->Init());

  CHECK_RESULT(pipe0->Init(queues[0], queues[1], pipe1));
  CHECK_RESULT(pipe1->Init(queues[1], queues[0], pipe0));
  pipes[0] = pipe0;
  pipes[1] = pipe1;
  return 0;
}

Pipe::Pipe(int index, bool enable_recycle)
  : listener_(nullptr)
  , terminated_(false)
  , peer_terminated_(false)
  , in_queue_(nullptr)
  , out_queue_(nullptr)
  , enable_recycle_(enable_recycle)
  , index_(index) {
  // nothing
}

Pipe::~Pipe() {
  if (nullptr != listener_) {
    listener_->OnDestroy(this);
  }
  // Only destroy the in queue, the out queue would be destroyed
  // by the peer pipe object
  if (nullptr != in_queue_) {
    delete in_queue_;
    in_queue_ = nullptr;
  }
  // delete data of the recycle queue 
  PipeMsg msg = nullptr;
  while (recycle_queue_.Read(&msg)) {
    delete msg;
  }
}

int Pipe::Init(MessageQueue* in_queue, MessageQueue* out_queue, 
  std::shared_ptr<Pipe> peer) {
  peer_ = peer;
  in_queue_ = in_queue;
  out_queue_ = out_queue;
  if (enable_recycle_) {
    CHECK_RESULT(recycle_queue_.Init());
  }
  return 0;
}

int Pipe::Write(PipeMsg msg, bool incomplete) {
  if (terminated_) {
    return EPOLL_FAIL;
  }
  CHECK_RESULT(out_queue_->Write(msg, incomplete));
  if (!incomplete) {
    if (!out_queue_->Flush()) {
      // activate the peer pipe reader
      peer_->SendReadActivated();
    }
  }
  return 0;
}

int Pipe::Write(PipeMsg msg, bool* o_wake_peer) {
  if (terminated_) {
    return EPOLL_FAIL;
  }
  CHECK_RESULT(out_queue_->Write(msg, false));
  *o_wake_peer = false;
  if (!out_queue_->Flush()) {
    *o_wake_peer = true;
    // activate the peer pipe reader
    peer_->SendReadActivated();
  }
  return 0;
}

int Pipe::Unwrite(PipeMsg* res_msg) {
  return out_queue_->Unwrite(res_msg) ? 0 : EPOLL_FAIL;
}

bool Pipe::CheckRead() {
  if (peer_terminated_) {
    return false;
  }
  return in_queue_->CheckRead();
}

int Pipe::Read(int timeout, PipeMsg* res_msg) {
  int res = 0;
  if (DoRead(res_msg, &res)) {
    return res;
  }
  if (0 == timeout) {
    // do not wait
    return EPOLL_FAIL;
  }
  { // lock context
    std::unique_lock<std::mutex> lock(mutex_);
    do {
      if (DoRead(res_msg, &res)) {
        return res;
      }
      if (timeout > 0) {
        condition_.wait_for(lock, std::chrono::seconds(timeout));
        if (DoRead(res_msg, &res)) {
          return res;
        }
      } else {
        // wait forever
        condition_.wait(lock);
      }
    } while (timeout < 0);
  }
  return EPOLL_FAIL;
}

int Pipe::Terminate() {
  if (terminated_) {
    return 0;
  }
  CHECK_RESULT(out_queue_->Write(nullptr, false));
  if (!out_queue_->Flush()) {
    // activate the peer pipe reader
    peer_->SendReadActivated();
  }
  terminated_ = true;
  // release reference to the peer as we would not need to write anymore
  peer_.reset();
  return 0;
}


void Pipe::SendReadActivated () {
  if (nullptr != listener_) {
    listener_->OnReadActivated(this);
  }
  { // lock context
    std::unique_lock<std::mutex> lock(mutex_);
    condition_.notify_all();
  }
}

bool Pipe::DoRead(PipeMsg* res_msg, int* res) {
  if (peer_terminated_) {
    *res = EPOLL_FAIL;
    return true;
  }
  if (in_queue_->Read(res_msg)) {
    if (nullptr == *res_msg) {
      peer_terminated_ = true;
      *res = EPOLL_EOF;
      return true;
    }
    *res = 0;
    return true;
  }
  return false;
}

int Pipe::Recycle(PipeMsg msg) {
  if (terminated_) {
    return EPOLL_FAIL;
  }
  if (!enable_recycle_) {
    return EPOLL_INVALID;
  }
  CHECK_RESULT(recycle_queue_.Write(msg, false));
  recycle_queue_.Flush();
  return 0;
}


bool Pipe::ConsumeRecycleQueue(PipeMsg* res_msg) {
  if (peer_terminated_) {
    return false;
  }
  if (!peer_->enable_recycle_) {
    return false;
  }
  if (peer_->recycle_queue_.Read(res_msg)) {
    if (nullptr == *res_msg) {
      return false;
    }
    return true;
  }
  return false;
}

