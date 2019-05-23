#pragma once

#include <variant>
#include <type_traits>
#include <cstdio>
#include <functional>
#include "meta.hpp"

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


template <typename TransitionTable, typename Context = bool>
class state_machine {
public:

    using extracted = decltype(extract_states(std::declval<TransitionTable>()));
    using states = remove_duplicates_t<extracted>;

    static constexpr size_t state_count = std::variant_size_v<TransitionTable>;
    static_assert(state_count > 1, "no state transitions in table");

    template <typename StartState>
    state_machine(StartState start, Context c = Context()) :
        m_ctx(c),
        m_state(start)
    {
        printf("transitions in table: %zd\n", state_count);
        printf("unique transitions: %zd\n", std::variant_size_v<states>);
    }

    template <typename Event>
    auto constexpr feed(Event evt) {


        //process(m_state, evt);

#if 0
        std::visit([&](auto && state){
            printf("doing a visitation on current state: %s\n", typeid(state).name());
            process(m_state, evt);
        }, m_state);
#endif

    }
//protected:
    template <typename State, typename Event>
    auto constexpr process(const State & /*current_state*/, Event && evt) {
        using state_type            = typename std::decay<State>::type;
        using event_type            = typename std::decay<Event>::type;
        using transition_state_t    = std::variant_alternative_t<match<TransitionTable, state_type, event_type>(), TransitionTable>; // transition_state_t is now a transition<x,y,z>
        using next_state_t          = typename transition_state_t::next_state;
        /*

*/
        /*
         * instantiate next state
         *
         */
        auto prev = m_state;

auto next = next_state_t(m_ctx);

        //instance = 1;
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

        next(evt);
        m_state = (states)next_state_t(m_ctx);

        /*result_type completion_handler = next(event, [next,this](){
            // we'd also need to store the completion_handler somewhere, but I can actually not
            // remember the actual asio semantics
        });
        */


        return m_state;
    }

    Context m_ctx;
    states m_state;
};
