/* wm_event.c */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* Copyright (C) 1990-1998 Colton Software Limited
 * Copyright (C) 1998-2014 R W Colton */

/* Centralised event acquisition and processing */

/* SKS October 1990 */

#include "common/gflags.h"

#include "wm_event.h"

#ifndef __cs_wimp_h
#include "cs-wimp.h"    /* includes wimp.h -> os.h, sprite.h */
#endif

#ifndef __wimpt_h
#include "wimpt.h"
#endif

#ifndef __cs_event_h
#include "cs-event.h"
#endif

#ifndef __cs_winx_h
#include "cs-winx.h"    /* includes win.h */
#endif

/******************************************************************************
*
* get and process events - returning only if foreground null events
* are wanted. centralises null processing, modeless dialog, accelerator
* translation and message dispatch
*
* first call from main program should be (void) wm_events_get(0);
*
******************************************************************************/

extern S32
wm_events_get(
    S32 fgNullEventsWanted)
{
    static S32 last_event_was_null = 0;

    #if RISCOS
    wimp_eventstr msg;
    #endif
    S32 peek, bgNullEventsWanted, bgNullTestWanted;
    S32 res;

    for(;;)
        {
        /* start exit sequence; anyone interested in setting up bgNulls
         * better do their thinking in the null query handlers
        */
        if(last_event_was_null)
            {
            /* only ask our null query handlers at the end of each real
             * requested null event whether they want to continue having nulls
             * or not; this saves wasted time querying after every single event
             * null or otherwise; so we must request a single test null event
             * to come through to query the handlers whenever some idle time turns up
            */
            bgNullEventsWanted = (Null_DoQuery() == NULL_EVENTS_REQUIRED);
            bgNullTestWanted   = FALSE;
            }
        else
            {
            bgNullEventsWanted = FALSE;
            bgNullTestWanted   = Null_HandlersPresent();
            }

        peek = (fgNullEventsWanted || bgNullEventsWanted || bgNullTestWanted);

        #if RISCOS
        /* enable null events to ourselves if needed here */

        /* note that other parts of RISC_OSLib etc may be trying
         * to enable nulls independently using event/win so take care
         * as it stands wimpt is using win_idle_event_claimer() to
         * reenable nulls too for win_idle clients
        */
        wimpt_poll((wimp_emask) (event_getmask() & ~(peek ? wimp_EMNULL : 0)),
                   &msg);
        /* loops itself till good event read */

        /* res = 0 only if we got a peek at a null event and we wanted it! */
        res = (!peek || (msg.e != wimp_ENULL));
        #endif

        /* if we have a real event, process it and look again */
        #if RISCOS
        /* always allow other clients to have a look in at nulls if they want */
        if((msg.e != wimp_ENULL)  ||  (win_idle_event_claimer() != win_IDLE_OFF))
            event_do_process(&msg);
        #endif

        if(res)
            {
            last_event_was_null = 0;
            continue;
            }

        /* only allow our processors a look in now at nulls they wanted */

        last_event_was_null = 1;

        /* perform the initial idle time test if some idle time has come */
        if(bgNullTestWanted)
            bgNullEventsWanted = (Null_DoQuery() == NULL_EVENTS_REQUIRED);

        /* processor is idle, dispatch null event appropriately */
        if(bgNullEventsWanted)
            (void) Null_DoEvent();

        if(fgNullEventsWanted)
            {
            /* we have our desired fg idle, so return */
            res = 0;
            break;
            }

        /* check bg null requirements to loop again and again */
        if(bgNullEventsWanted)
            continue;

        /* exiting strangely */
        res = 0;
        break;
        }

    return(res);
}

/* end of wm_event.c */
