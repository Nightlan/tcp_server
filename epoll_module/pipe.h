/**
* epoll_module
* Copyright (c) 2017 engwei, yang (437798348@qq.com).
*
* @version 1.0
* @author engwei, yang
*/

/// @file Defines the pipe class to exchange messages between two threads.

#ifndef EPOLL_PIPE_H__
#define EPOLL_PIPE_H__

#include <common.h>
#include <spsc_queue.h>
#include <thread>
#include <mutex>
#include <condition_variable>

class Pipe;             // Declare Pipe

/// The listener for the pipe events
class PipeEventListener {
 public:
  // Default empty virtual destructor
  virtual ~PipeEventListener() {}
  // Get called when new message arrived on the corresponding pipe object
  // so that it changed the state to read activated
  virtual void OnReadActivated(Pipe* /*pipe*/) = 0;
  // Get called when this pipe is about to be destroyed
  virtual void OnDestroy(Pipe* /*pipe*/) = 0;
};

// The function to create a pair of pipe objects. The two created pipe
// objects are intended to be operated on two threads, and when write
// message on any of the two pipes, the other pipe would read this message
int CreatePipePair(std::shared_ptr<Pipe> pipes[2], int index, bool enable_recycle);

/// The pipe class used to exchange messages between two threads. This
/// class is only intended to be created through the CreatePipePair
/// function
typedef void* PipeMsg;

class Pipe {
 public:
  //  This allows CreatePipePair to create Pipe objects.
  friend int CreatePipePair(std::shared_ptr<Pipe> pipes[2], int index, bool enable_recycle);

  // Destroy the pipe
  ~Pipe();
  
  // Writes message to the pipe
  int Write(PipeMsg msg, bool incomplete = false);

  // writes message to the pipe and return whether we need to awake the peer
  int Write(PipeMsg msg, bool* o_wake_peer);

  //  Pop an incomplete message from the pipe.
  int Unwrite(PipeMsg* res_msg);

  //  Check whether item is available for reading.
  bool CheckRead();

  // Reads message from the pipe
  int Read(int timeout, PipeMsg* res_msg);

  // Ask pipe to terminate. 
  int Terminate();

  // Recycle msg
  int Recycle(PipeMsg msg);

  // consume recycle queue
  bool ConsumeRecycleQueue(PipeMsg* res_msg);

  // Sets the listener
  void set_listener(PipeEventListener* listener) {
    // could only be set once
    listener_ = listener;
  }

  int index() const {
    return index_;
  }

 private:
  static const int kIdleMessageGranularity = 256;
  // Define the message queue
  typedef SpscQueue<PipeMsg, kIdleMessageGranularity> MessageQueue;

  //  Constructor is private. pipe can only be created using
  //  PipePair function.
  explicit Pipe(int index, bool enable_recycle);

  int Init(MessageQueue* in_queue, MessageQueue* out_queue, 
    std::shared_ptr<Pipe> peer);

  // Notify this pipe new message arrived
  void SendReadActivated ();

  bool DoRead(PipeMsg* res_msg, int* res);

  // index of this pipe pair
  int index_;
  PipeEventListener* listener_;
  // The flag indicating whether this pipe has been terminated
  bool terminated_;
  bool peer_terminated_;
  bool enable_recycle_;
  // Hold reference to the peer object so that the peer
  // object would not be destroyed when calling SendReadActivated
  // on it
  std::shared_ptr<Pipe> peer_;

  // The input queue
  MessageQueue* in_queue_;
  // The output queue
  MessageQueue* out_queue_;
  // The recycle queue
  MessageQueue recycle_queue_;
  // mutex for read waiting
  std::mutex mutex_;
  // condition for read waiting
  std::condition_variable condition_;
  // Disable copying of Pipe
  DISALLOW_CONSTRUCTORS(Pipe);
};

#endif // EPOLL_PIPE_H__
