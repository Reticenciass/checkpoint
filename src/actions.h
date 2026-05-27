#pragma once
#ifndef ACTIONS_H
#define ACTIONS_H

#include "config.h"

/* Execute the action associated with the given bind. */
void action_execute(const Bind *b);

/* Focus a window whose title contains 'fragment' (case-insensitive). */
BOOL action_focus_window(const WCHAR *fragment);

#endif /* ACTIONS_H */
