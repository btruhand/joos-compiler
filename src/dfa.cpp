#include "dfa.h"
#include "states.h"
#include <assert.h>

Dfa::Dfa()
{
    current_state = DS_START;
}

int Dfa::transition(char c)
{
    assert(dfa.find(current_state) != dfa.end());

    current_state = dfa[current_state].second(c);

    return 0;
}

int Dfa::onAcceptingState()
{
    assert(dfa.find(current_state) != dfa.end());

    return dfa[current_state].first == DS_ACCEPT;
}

void Dfa::resetDfa()
{
    current_state = DS_START;
}
