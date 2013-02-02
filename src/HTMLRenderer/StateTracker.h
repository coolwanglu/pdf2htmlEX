/*
 * StateTracker.h
 *
 * track specific PDF state
 * manage reusable CSS classes
 *
 * Copyright (C) 2013 Lu Wang <coolwanglu@gmail.com>
 */

#ifndef STATE_TRACKER_H__
#define STATE_TRACKER_H__

#include <GfxState.h>


namespace pdf2htmlEX {

class StateTrackerBase
{
public:
    virtual ~StateTracker(void);

    // usually called at the beginning of a page
    virtual void reset(void) = 0;
    /*
     * update() is called when PDF updates the state
     * check_state() is called when we really need the new state
     *
     * There could be a number of calls to update() between 2 consecutive calls to check_state()
     * So usually we just mark as changed in update, and actually retrive from the state in check_state()
     */
    virtual void update(GfxState * state) = 0;
    /*
     * return if state has been updated
     *
     * if force is true, do check the state if there is no update() called before
     * useful for updateAll
     */
    virtual bool check_state(GfxState * state, bool force) = 0;

    long long get_id() const { return id; }

protected:
    long long id; // For CSS classe
};


} // namespace pdf2htmlEX 

#endif //STATE_TRACKER_H__
