#pragma once

#include <variant>
#include <type_traits>
#include <cstdio>
#include <functional>
#include <thread>
#include <chrono>
#include <optional>

#include "meta.hpp"
#include "type_name.hpp"

/*
 * transition just stores the types used in transitions
 */
template <typename Entry, typename Event, typename Next>
struct transition {
    using entry_state = Entry;
    using event       = Event;
    using next_state  = Next;
};


namespace {
template <typename T, typename U, typename V>
constexpr bool equals() {
    return (std::is_same_v<typename T::entry_state, U>) && (std::is_same_v<typename T::event, V>);
}

template<typename VariantType, typename U, typename V, std::size_t index = 0>
constexpr std::size_t match() {
    if constexpr (index == std::variant_size_v<VariantType>) {
        return index;
    } else if constexpr (equals<std::variant_alternative_t<index, VariantType>, U, V>()) {
        return index;
    } else {
        return match<VariantType, U, V, index + 1>();
    }
}

} // namespace anon


template <template <class...> class TT, class ... Ts>
auto extract_states(TT<Ts...>)
-> TT<typename Ts::entry_state..., typename Ts::next_state...>;

template <template <class...> class TT, class ... Ts>
auto extract_events(TT<Ts...>)
-> TT<typename Ts::event...>;

// Terminal state marker - specialize this for states that don't need transitions
template <typename T>
struct is_terminal_state : std::false_type {};

// Compile-time transition completeness checker
namespace detail {
    template <typename State, typename Event, typename TransitionTable, std::size_t index = 0>
    constexpr bool has_transition() {
        if constexpr (index == std::variant_size_v<TransitionTable>) {
            return false;
        } else if constexpr (equals<std::variant_alternative_t<index, TransitionTable>, State, Event>()) {
            return true;
        } else {
            return has_transition<State, Event, TransitionTable, index + 1>();
        }
    }

    template <typename State, typename EventList, typename TransitionTable, std::size_t index = 0>
    constexpr bool check_state_complete() {
        if constexpr (is_terminal_state<State>::value) {
            return true; // Terminal states don't need transitions
        } else if constexpr (index == std::variant_size_v<EventList>) {
            return true; // All events checked
        } else {
            using Event = std::variant_alternative_t<index, EventList>;
            if constexpr (!has_transition<State, Event, TransitionTable>()) {
                return false; // Missing transition found
            } else {
                return check_state_complete<State, EventList, TransitionTable, index + 1>();
            }
        }
    }

    template <typename StateList, typename EventList, typename TransitionTable, std::size_t index = 0>
    constexpr bool check_all_transitions() {
        if constexpr (index == std::variant_size_v<StateList>) {
            return true;
        } else {
            using State = std::variant_alternative_t<index, StateList>;
            if constexpr (!check_state_complete<State, EventList, TransitionTable>()) {
                return false;
            } else {
                return check_all_transitions<StateList, EventList, TransitionTable, index + 1>();
            }
        }
    }
}


template <typename TransitionTable, typename Context = bool>
class state_machine {
public:

    using extracted = decltype(extract_states(std::declval<TransitionTable>()));
    using states = remove_duplicates_t<extracted>;

    using extracted_events = decltype(extract_events(std::declval<TransitionTable>()));
    using events = remove_duplicates_t<extracted_events>;

    static constexpr size_t state_count = std::variant_size_v<TransitionTable>;
    static_assert(state_count > 1, "no state transitions in table");

    // Compile-time check: All non-terminal states must have transitions for all events
    static_assert(detail::check_all_transitions<states, events, TransitionTable>(),
                  "ERROR: Incomplete transition table! Some states are missing transitions for certain events. "
                  "Either add the missing transitions or mark the state as terminal using is_terminal_state<YourState>.");

    state_machine(Context c = Context()) :
        m_ctx(c)
    {
//        printf("transitions in table: %zd\n", state_count);
//        printf("unique transitions: %zd\n", std::variant_size_v<states>);
    }

    // Generate Mermaid flowchart
    template <std::size_t index = 0>
    static void print_mermaid_transitions() {
        if constexpr (index < std::variant_size_v<TransitionTable>) {
            using T = std::variant_alternative_t<index, TransitionTable>;
            using from = typename T::entry_state;
            using event = typename T::event;
            using to = typename T::next_state;

            printf("    %s -->|%s| %s\n",
                   type_name<from>().c_str(),
                   type_name<event>().c_str(),
                   type_name<to>().c_str());

            print_mermaid_transitions<index + 1>();
        }
    }

    static void print_mermaid() {
        printf("```mermaid\nstateDiagram-v2\n");

        // Mark terminal states
        printf("    [*] --> start\n");
        print_mermaid_transitions();

        // Mark terminal states with end markers
        if constexpr (std::variant_size_v<states> > 0) {
            print_terminal_states_mermaid<0>();
        }

        printf("```\n");
    }

    template <std::size_t index = 0>
    static void print_terminal_states_mermaid() {
        if constexpr (index < std::variant_size_v<states>) {
            using State = std::variant_alternative_t<index, states>;
            if constexpr (is_terminal_state<State>::value) {
                printf("    %s --> [*]\n", type_name<State>().c_str());
            }
            print_terminal_states_mermaid<index + 1>();
        }
    }

    // Generate Graphviz DOT format
    template <std::size_t index = 0>
    static void print_dot_transitions() {
        if constexpr (index < std::variant_size_v<TransitionTable>) {
            using T = std::variant_alternative_t<index, TransitionTable>;
            using from = typename T::entry_state;
            using event = typename T::event;
            using to = typename T::next_state;

            printf("    \"%s\" -> \"%s\" [label=\"%s\"];\n",
                   type_name<from>().c_str(),
                   type_name<to>().c_str(),
                   type_name<event>().c_str());

            print_dot_transitions<index + 1>();
        }
    }

    static void print_graphviz() {
        printf("digraph StateMachine {\n");
        printf("    rankdir=LR;\n");
        printf("    node [shape=circle];\n");
        printf("    start [shape=circle, style=filled, fillcolor=lightgreen];\n");

        // Mark terminal states
        if constexpr (std::variant_size_v<states> > 0) {
            print_terminal_states_dot<0>();
        }

        printf("\n");
        print_dot_transitions();
        printf("}\n");
    }

    template <std::size_t index = 0>
    static void print_terminal_states_dot() {
        if constexpr (index < std::variant_size_v<states>) {
            using State = std::variant_alternative_t<index, states>;
            if constexpr (is_terminal_state<State>::value) {
                printf("    \"%s\" [shape=doublecircle, style=filled, fillcolor=lightcoral];\n",
                       type_name<State>().c_str());
            }
            print_terminal_states_dot<index + 1>();
        }
    }

    using Callback = std::function<void(const std::error_code &ec)>;

    template <typename StartState, typename ... Args>
    void start(Args && ... args) {
        m_state = StartState(m_ctx, std::forward<Args>(args)...);
        std::get<StartState>(*m_state)([this](auto && arg){
            push(arg);
        });
    }

protected:
    template <typename Event>
    void push(Event evt) {

        std::visit([&](auto && current_state){
            using current_state_type = std::decay_t<decltype(current_state)>;
//            printf("*** current state is: %s, event is: %s ***\n",
//                   type_name<current_state_type>().c_str(),
//                   type_name<Event>().c_str()
//                   );

            if constexpr(std::variant_size_v<TransitionTable> > match<TransitionTable, current_state_type, Event>()) {
                //using transition_state_type = std::variant_alternative_t<match<TransitionTable, current_state_type, Event>(), TransitionTable>;
                //using next_state_type       = typename transition_state_type::next_state;
                process(current_state, evt);
            } else {
                printf("no transition for: %s + %s\n", type_name<current_state_type>().c_str(), type_name<Event>().c_str());
                if (m_cb) {
                    m_cb({});
                }
            }

        }, m_state.value());
    }
//protected:



    template <typename State, typename Event>
    auto constexpr /* __attribute__((deprecated))*/ process(const State & /*current_state*/, Event && evt)  {



        using state_type            = typename std::decay<State>::type;
        using event_type            = typename std::decay<Event>::type;

        static_assert(std::variant_size_v<TransitionTable> > match<TransitionTable, state_type, event_type>(), "no such transition");


        using transition_state_t    = std::variant_alternative_t<match<TransitionTable, state_type, event_type>(), TransitionTable>; // transition_state_t is now a transition<x,y,z>
        using next_state_t          = typename transition_state_t::next_state;

        // instantiate next state, saving previous
        auto prev = std::exchange(m_state, next_state_t(m_ctx));


        printf("[%s + %s > %s]\n", type_name<state_type>().c_str(), type_name<event_type>().c_str(), type_name<next_state_t>().c_str());
        std::this_thread::sleep_for(std::chrono::seconds(1));


         // perform the transition / action
        std::get<next_state_t>(*m_state)(evt, [&](auto && args) {
            // this might be a good place to check for termination
            push(args);
        });


        //return std::get<next_state_t>(m_state); <- does not work, since m_state may have been changed mean-while!
    }

    Context m_ctx;
    std::optional<states> m_state;
    Callback m_cb;
};
