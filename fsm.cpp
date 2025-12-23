
#include <stdio.h>
#include <cstdio>
#include <memory>
#include <system_error>
#include <variant>
#include <any>
#include <functional>
#include <type_traits>

#include "fsm.hpp"

template <typename result>
struct state {};
using sock = int;



// input events, for now just success and fail
template <typename T = std::string>
struct success {
    using value_type = T;

    success() = default;
    success(T v = T()) :
        value(v)
    {}

    T value;
};

using exception = std::runtime_error;


// context for states, wrapping up common stuff shared across all states, if really needed
struct context {
    void log(const std::string &what) {
        printf("* %s\n", what.c_str());
    }
};


/************************************************************************
 * state definitions
 */

using SharedContext = std::shared_ptr<context>;
//
// start state, does nothing
//
struct start {
    start(SharedContext ctx, std::string ip, std::string user, std::string pass) :
        m_ctx(ctx)
    {}

    template <typename Callable>
    void operator()(Callable && cb) {
        success<int> e(42);
        cb(e);
    }

    SharedContext m_ctx;
};


//
// connecting state: connects a socket to a given host
//
struct connecting {
    connecting(std::shared_ptr<context> ctx) :
        m_ctx(ctx)
    {}

    // event and callback function are being passed in
    template <typename Callable>
    void operator()(success<sock> s, Callable && cb) {
        //m_ctx->log(std::string("connecting: starting the connection...") + std::to_string(s.value));
        cb(s);
    }

protected:
    std::shared_ptr<context> m_ctx;
};

// connected state: sends some initial data over the socket
struct connected {
    connected(std::shared_ptr<context> ctx) : m_ctx(ctx) {}

    template <typename Callable>
    void operator()(success<sock>, Callable && cb) {
        cb(std::runtime_error("remote disconnect"));
    }

    std::shared_ptr<context> m_ctx;
};

struct disconnected {};

struct failed {
    failed(std::shared_ptr<context> ctx) : m_ctx(ctx) {}

    template <typename Callable>
    void operator()(exception e, Callable && cb) {
        m_ctx->log(std::string("failed: ") + e.what());
        cb(e);
    }
    SharedContext m_ctx;
};

// Mark terminal states - these don't need outgoing transitions
template <> struct is_terminal_state<failed> : std::true_type {};
template <> struct is_terminal_state<disconnected> : std::true_type {};
template <> struct is_terminal_state<connected> : std::true_type {};


/*
 * obviously, we could introduce other kinds of structs
 * into the transitions, like explicit start/stop states
 * any handle std::any as a wildcard event
 */

using transitions = std::variant<
/*  ---------- | state      | event ------------| followup-state -- */
    transition  <start,       success<sock>,      connecting>,
    transition  <start,       exception,          failed>,

    transition  <connecting,  success<sock>,      connected>,
    transition  <connecting,  exception,          failed>,

    transition  <connected,   exception,          failed>

>;

//using states = remove_duplicates_t<decltype(extract_states(table))>;


int main(int, char **) {
    SharedContext ctx = std::make_shared<context>();
    state_machine<transitions, SharedContext> fsm(ctx);

    fsm.start<start>("10.0.0.50", "user", "pass");

    // fsm.start<connecting>(); <- does not work due to operator()() design, good!

    //fsm(exception("foo"));
    printf("terminated\n");
}
