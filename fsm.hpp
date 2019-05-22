#pragma once

#include <variant>
#include <type_traits>

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
    if  (index == std::variant_size_v<VariantType>) {
        return index;
    } else if (equals<std::variant_alternative_t<index, VariantType>, U, V>()) {
        return index;
    } else {
        return match<VariantType, U, V, index + 1>();
    }
}

} // namespace anon





template <typename TransitionTable, typename Context = bool>
class state_machine {
public:
    static constexpr size_t state_count = std::variant_size_v<TransitionTable>;
    static_assert(state_count > 1, "no state transitions in table");

    state_machine(Context c = Context()) :
        m_ctx(c)
    {
        printf("transitions in table: %zd\n", state_count);
    }

    template <typename StartState, typename Event, class CompletionToken >
    auto start(Event evt, CompletionToken token) {
        //push<StartState>(evt);
    }

public:
    template <typename State, typename Event>
    auto operator()(const State & current_state, Event & event) {

        using state_type            = typename std::decay<State>::type;
        using event_type            = typename std::decay<Event>::type;
        using transition_state_t    = std::variant_alternative_t<match<TransitionTable, state_type, event_type>(), TransitionTable>; // transition_state_t is now a transition<x,y,z>
        using next_state_t          = typename transition_state_t::next_state;


        /*
         * instantiate next state
         *
         */
        next_state_t next(m_ctx);
        printf("===== state: '%s', event: '%s', next ==> '%12s'\n", typeid(State).name(), typeid(Event).name(), typeid(next_state_t).name());


        /*
        using cb_t = std::function<void()>;
        using result_type = std::invoke_result_t<next_state_t, Event, cb_t>;
        */

        /*
         * perform the transition / action
         *
         * Sidemark: IDK if the functor is needed or not.
         */

        /*result_type completion_handler =*/ next(event, [next,this](){
            // we'd also need to store the completion_handler somewhere, but I can actually not
            // remember the actual asio semantics
        });


        return next;
    }

    Context m_ctx;
};
