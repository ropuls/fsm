
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

//
// start state, does nothing
//
struct start {};


//
// connecting state: connects a socket to a given host
//
struct connecting {
    connecting(std::shared_ptr<context> ctx) :
        m_ctx(ctx)
    {}

    // event and callback function are being passed in
    void operator()(success<sock> s) {
        m_ctx->log(std::string("connecting: starting the connection...") + std::to_string(s.value));
    }

protected:
    std::shared_ptr<context> m_ctx;
};

// connected state: sends some initial data over the socket
struct connected {
    connected(std::shared_ptr<context> ctx) :
        m_ctx(ctx)
    {
        m_ctx->log("connected: created");
    }

    void operator()(success<sock>) {
        m_ctx->log("connected: sending credentials");
    }
    std::shared_ptr<context> m_ctx;
};

struct disconnected {};

struct failed {
    failed(std::shared_ptr<context> ctx) :
        m_ctx(ctx)
    {}

    void operator()(exception e) {
        m_ctx->log(std::string("failed: something whent wrong, namely: ") + e.what());
        exit(1);
    }
    std::shared_ptr<context> m_ctx;
};


using transitions = std::variant<
/*  ---------- | state      | event ------------| followup-state -- */
    transition  <start,       success<sock>,      connecting>,
    transition  <start,       exception,          failed>
/*
    transition  <connecting,  success<sock>,      connected>,
    transition  <connected,   exception,          failed>
    */
>;

//using states = remove_duplicates_t<decltype(extract_states(table))>;


int main(int, char *argv[]) {
    state_machine<transitions, std::shared_ptr<context>> fsm;

    start start_token;
    success<sock> sck(0);

    auto next = fsm(start_token, sck);

    printf("terminated\n");
}
