#ifndef CPPURSES_SYSTEM_EVENT_LOOP_HPP
#define CPPURSES_SYSTEM_EVENT_LOOP_HPP
#include <atomic>
#include <future>
#include <thread>

#include <cppurses/painter/detail/paint_middleman.hpp>
#include <cppurses/painter/detail/staged_changes.hpp>
#include <cppurses/system/detail/event_invoker.hpp>
#include <cppurses/system/detail/event_queue.hpp>

namespace cppurses {

class Event_loop {
   public:
    Event_loop() = default;

    Event_loop(const Event_loop&) = delete;
    Event_loop& operator=(const Event_loop&) = delete;

    // exit_ member is not default movable
    Event_loop(Event_loop&&);
    Event_loop& operator=(Event_loop&&);

    /// Makes sure the loop has exited and returned from async functions.
    virtual ~Event_loop();

    /// Start the event loop.
    int run();

    /// Start the event loop in a separate thread.
    void run_async();

    /// Calls on the loop to exit at the next exit point.
    /** The return code value is used when returning from run() or wait(). This
     *  function is thread safe. */
    void exit(int return_code);

    /// Blocks until the async event loop returns.
    /** Event_loop::exit(int) must be called to return from wait().
     *  @return the return code passed to the call to exit(). */
    int wait();

    /// Returns the thread ID of thread that launched this loop.
    /** Used for posting events to the correct queue in System::post_event() */
    std::thread::id get_thread_id() const;

    /// Returns the Staged_changes of this loop/thread.
    const detail::Staged_changes& staged_changes() const;

    /// Returns the Staged_changes of this loop/thread.
    detail::Staged_changes& staged_changes();

   protected:
    /// Override this in derived classes to define Event_loop behavior.
    /** This function will be called on once every loop iteration. It is
     *  expected that is will post an event to the Event_queue. After this
     *  function is called, the Event_queue is invoked, and then staged changes
     *  for this Event_loop are flushed to the screen, and the loop begins
     *  again. */
    virtual void loop_function() = 0;

   private:
    void process_events();

    std::future<int> fut_;
    std::thread::id thread_id_;
    int return_code_{0};
    std::atomic<bool> exit_{false};
    bool running_{false};

    detail::Event_queue event_queue_;
    detail::Event_invoker invoker_;

    detail::Staged_changes staged_changes_;
    detail::Paint_middleman paint_middleman_;

    friend class System;
};

}  // namespace cppurses
#endif  // CPPURSES_SYSTEM_EVENT_LOOP_HPP
