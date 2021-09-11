#pragma once


#include <boost/any.hpp>  // for any
#include <boost/lockfree/spsc_queue.hpp>
#include <iostream>

#include "mgr/ObjectManager.h"

namespace whale {
namespace vision {
// we use boost::lockfree::spsc_queue implement, we also can use

// or std::queue
template <typename T>
class Queue {
 public:
    Queue(int size, bool wait = true) : wait_(wait), cap_(size) {
        queue_impl_ = std::make_unique<boost::lockfree::spsc_queue<T>>(size);
    }
    bool Push(T&& t, bool block = true) {
        // todo default lost some data.
        if (Size() >= cap_) {
            // LOG(INFO) << " common cap_ full " ;
            return false;
        }
        return queue_impl_->push(std::move(t));
    }
    bool Pop(T& t, bool block = true) {
        queue_impl_->pop(t);
        return true;
    }
    bool Empty(){return queue_impl_->empty();}
    void setWaitStatus(bool wait) { wait_ = wait; }
    int Size() { return queue_impl_->size(); }
    void setCapicaty(int cap) { cap_ = cap; }

 private:
    bool wait_;
    int cap_;
    std::unique_ptr<boost::lockfree::spsc_queue<T>> queue_impl_;
};

template <>
class Queue<boost::any> {
 public:
    Queue(int size, bool wait = true) : wait_(wait), cap_(size) {
        queue_impl_ = std::make_unique<boost::lockfree::spsc_queue<boost::any>>(size);
    }
    bool Push(boost::any&& t, bool block = true) {
        if (Size() >= cap_) {
            // todo default lost some data.
            //  LOG(INFO) << " this queue is full, now lost it.... " ;
            return false;
        }
        return queue_impl_->push(std::move(t));
    }
    bool Pop(boost::any& t, bool block = true) {
        queue_impl_->pop(t);
        return true;
    }
    bool Empty() {return queue_impl_->empty();}
    void setWaitStatus(bool wait) { wait_ = wait; }
    int Size() { return queue_impl_->read_available(); }
    void setCapicaty(int cap) { cap_ = cap; }

 private:
    bool wait_;
    int cap_;
    std::unique_ptr<boost::lockfree::spsc_queue<boost::any>> queue_impl_;
};
using QueuePtr = std::shared_ptr<Queue<boost::any>>;

class QueueManager : public SafeObjectManager<QueueManager, QueuePtr> {};
}  // namespace vision
}  // namespace whale