/**
 * \file
 *
 * \brief Timeout driver.
 *
 (c) 2018 Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms,you may use this software and
    any derivatives exclusively with Microchip products.It is your responsibility
    to comply with third party license terms applicable to your use of third party
    software (including open source software) that may accompany Microchip software.

    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
    WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
    PARTICULAR PURPOSE.

    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
    BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
    FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
    ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
    THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 */

/**
 \defgroup doc_driver_timer_timeout Timeout Driver
 \ingroup doc_driver_timer

 \section doc_driver_timer_timeout_rev Revision History
 - v0.0.0.1 Initial Commit

@{
*/
#include <stdio.h>
#include "timeout.h"
#include "atomic.h"

absolutetime_t scheduler_dummy_handler(void *payload)
{
	return 0;
};

void           scheduler_start_timer_at_head(void);
void           scheduler_enqueue_callback(timer_struct_t *timer);
void           scheduler_set_timer_duration(absolutetime_t duration);
absolutetime_t scheduler_make_absolute(absolutetime_t timeout);
absolutetime_t scheduler_rebase_list(void);

timer_struct_t *scheduler_list_head                   = NULL;
timer_struct_t *volatile scheduler_execute_queue_head = NULL;

timer_struct_t          scheduler_dummy                         = {scheduler_dummy_handler};
volatile absolutetime_t scheduler_absolute_time_of_last_timeout = 0;
volatile absolutetime_t scheduler_last_timer_load               = 0;
volatile bool           scheduler_is_running                    = false;

void scheduler_timeout_init(void)
{

	while (RTC.STATUS > 0) { /* Wait for all register to be synchronized */
	}

	// RTC.CMP = 0x0; /* Compare: 0x0 */

	// RTC.CNT = 0x0; /* Counter: 0x0 */

	RTC.CTRLA = RTC_PRESCALER_DIV1_gc   /* 1 */
	            | 1 << RTC_RTCEN_bp     /* Enable: enabled */
	            | 0 << RTC_RUNSTDBY_bp; /* Run In Standby: disabled */

	// RTC.PER = 0xffff; /* Period: 0xffff */

	RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; /* 32KHz divided by 32 */

	// RTC.DBGCTRL = 0 << RTC_DBGRUN_bp; /* Run in debug: disabled */

	RTC.INTCTRL = 0 << RTC_CMP_bp    /* Compare Match Interrupt enable: disabled */
	              | 1 << RTC_OVF_bp; /* Overflow Interrupt enable: enabled */

	// RTC.PITCTRLA = RTC_PERIOD_OFF_gc /* Off */
	//		 | 0 << RTC_PITEN_bp; /* Enable: disabled */

	// RTC.PITDBGCTRL = 0 << RTC_DBGRUN_bp; /* Run in debug: disabled */

	// RTC.PITINTCTRL = 0 << RTC_PI_bp; /* Periodic Interrupt: disabled */
}

// Disable all the timers without deleting them from any list. Timers can be
//    restarted by calling startTimerAtHead
void scheduler_stop_timeouts(void)
{
	RTC.INTCTRL &= ~RTC_OVF_bm;
	scheduler_absolute_time_of_last_timeout = 0;
	scheduler_is_running                    = 0;
}

inline void scheduler_set_timer_duration(absolutetime_t duration)
{
	scheduler_last_timer_load = 65535 - duration;
	// Wait for clock domain synchronization
	while (RTC.STATUS & RTC_CNTBUSY_bm)
		;
	RTC.CNT = scheduler_last_timer_load;
}

// Convert the time provided from a "relative to now" time to a absolute time which
//    means ticks since the last timeout occurred or the timer module was started
inline absolutetime_t scheduler_make_absolute(absolutetime_t timeout)
{
	timeout += scheduler_absolute_time_of_last_timeout;
	if (scheduler_is_running) {
		uint32_t timerVal = RTC.CNT;
		if ((RTC.INTFLAGS & RTC_OVF_bm)) {
			timerVal = (65535);
		}
		timeout += timerVal - scheduler_last_timer_load;
	}
	return timeout;
}

// Adjust the time base so that we can never wrap, saving us a lot of complications
inline absolutetime_t scheduler_rebase_list(void)
{
	timer_struct_t *base_point = scheduler_list_head;
	absolutetime_t  base       = scheduler_make_absolute(0);

	while (base_point != NULL) {
		base_point->absolute_time -= base;
		base_point = base_point->next;
	}

	scheduler_absolute_time_of_last_timeout -= base;
	return base;
}

inline void scheduler_print_list(void)
{
	timer_struct_t *base_point = scheduler_list_head;
	while (base_point != NULL) {
		printf("%ld -> ", (uint32_t)base_point->absolute_time);
		base_point = base_point->next;
	}
	printf("NULL\n");
}

// Returns true if the insert was at the head, false if not
bool scheduler_sorted_insert(timer_struct_t *timer)
{
	absolutetime_t  timer_absolute_time = timer->absolute_time;
	uint8_t         at_head             = 1;
	timer_struct_t *insert_point        = scheduler_list_head;
	timer_struct_t *prev_point          = NULL;
	timer->next                         = NULL;

	if (timer_absolute_time < scheduler_absolute_time_of_last_timeout) {
		timer_absolute_time += 65535 - scheduler_rebase_list() + 1;
		timer->absolute_time = timer_absolute_time;
	}

	while (insert_point != NULL) {
		if (insert_point->absolute_time > timer_absolute_time) {
			break; // found the spot
		}
		prev_point   = insert_point;
		insert_point = insert_point->next;
		at_head      = 0;
	}

	if (at_head == 1) // the front of the list.
	{
		scheduler_set_timer_duration(65535);
		RTC.INTFLAGS = RTC_OVF_bm;

		timer->next         = (scheduler_list_head == &scheduler_dummy) ? scheduler_dummy.next : scheduler_list_head;
		scheduler_list_head = timer;
		return true;
	} else // middle of the list
	{
		timer->next = prev_point->next;
	}
	prev_point->next = timer;
	return false;
}

void scheduler_start_timer_at_head(void)
{
	RTC.INTCTRL &= ~RTC_OVF_bm;

	if (scheduler_list_head == NULL) // no timeouts left
	{
		scheduler_stop_timeouts();
		return;
	}

	absolutetime_t period = ((scheduler_list_head != NULL) ? (scheduler_list_head->absolute_time) : 0)
	                        - scheduler_absolute_time_of_last_timeout;

	// Timer is too far, insert dummy and schedule timer after the dummy
	if (period > 65535) {
		scheduler_dummy.absolute_time = scheduler_absolute_time_of_last_timeout + 65535;
		scheduler_dummy.next          = scheduler_list_head;
		scheduler_list_head           = &scheduler_dummy;
		period                        = 65535;
	}

	scheduler_set_timer_duration(period);

	RTC.INTCTRL |= RTC_OVF_bm;
	scheduler_is_running = 1;
}

// Cancel and remove all active timers
void scheduler_timeout_flush_all(void)
{
	scheduler_stop_timeouts();

	while (scheduler_list_head != NULL)
		scheduler_timeout_delete(scheduler_list_head);

	while (scheduler_execute_queue_head != NULL)
		scheduler_timeout_delete(scheduler_execute_queue_head);
}

// Deletes a timer from a list and returns true if the timer was found and
//     removed from the list specified
bool scheduler_timeout_delete_helper(timer_struct_t *volatile *list, timer_struct_t *timer)
{
	bool ret_val = false;
	if (*list == NULL)
		return ret_val;

	// Guard in case we get interrupted, we cannot safely compare/search and get interrupted
	RTC.INTCTRL &= ~RTC_OVF_bm;

	// Special case, the head is the one we are deleting
	if (timer == *list) {
		*list   = (*list)->next; // Delete the head
		ret_val = true;
		scheduler_start_timer_at_head(); // Start the new timer at the head
	} else {                             // More than one timer in the list, search the list.
		timer_struct_t *find_timer = *list;
		timer_struct_t *prev_timer = NULL;
		while (find_timer != NULL) {
			if (find_timer == timer) {
				prev_timer->next = find_timer->next;
				ret_val          = true;
				break;
			}
			prev_timer = find_timer;
			find_timer = find_timer->next;
		}
		RTC.INTCTRL |= RTC_OVF_bm;
	}

	return ret_val;
}

// This will cancel/remove a running timer. If the timer is already expired it will
//     also remove it from the callback queue
void scheduler_timeout_delete(timer_struct_t *timer)
{
	if (!scheduler_timeout_delete_helper(&scheduler_list_head, timer)) {
		scheduler_timeout_delete_helper(&scheduler_execute_queue_head, timer);
	}

	timer->next = NULL;
}

// Moves the timer from the active list to the list of timers which are expired and
//    needs their callbacks called in call_next_callback
inline void scheduler_enqueue_callback(timer_struct_t *timer)
{
	timer_struct_t *tmp;
	timer->next = NULL;

	// Special case for empty list
	if (scheduler_execute_queue_head == NULL) {
		scheduler_execute_queue_head = timer;
		return;
	}

	// Find the end of the list and insert the next expired timer at the back of the queue
	tmp = scheduler_execute_queue_head;
	while (tmp->next != NULL)
		tmp = tmp->next;

	tmp->next = timer;
}

// This function checks the list of expired timers and calls the first one in the
//    list if the list is not empty. It also reschedules the timer if the callback
//    returned a value greater than 0
// It is recommended this is called from the main superloop (while(1)) in your code
//    but by design this can also be called from the timer ISR. If you wish callbacks
//    to happen from the ISR context you can call this as the last action in timeout_isr
//    instead.
void scheduler_timeout_call_next_callback(void)
{

	if (scheduler_execute_queue_head == NULL)
		return;

	bool tempIE = (RTC.INTCTRL & RTC_OVF_bm) >> RTC_OVF_bp;
	RTC.INTCTRL &= ~RTC_OVF_bm;
	timer_struct_t *callback_timer = scheduler_execute_queue_head;

	// Done, remove from list
	scheduler_execute_queue_head = scheduler_execute_queue_head->next;
	// Mark the timer as not in use
	callback_timer->next = NULL;
	if (tempIE) {
		RTC.INTCTRL |= RTC_OVF_bm;
	}

	absolutetime_t reschedule = callback_timer->callback_ptr(callback_timer->payload);

	// Do we have to reschedule it? If yes then add delta to absolute for reschedule
	if (reschedule) {
		scheduler_timeout_create(callback_timer, reschedule);
	}
}

// This function starts the timer provided with an expiry equal to "timeout".
// If the timer was already active/running it will be replaced by this and the
//    old (active) timer will be removed/cancelled first
void scheduler_timeout_create(timer_struct_t *timer, absolutetime_t timeout)
{
	// If this timer is already active, replace it
	scheduler_timeout_delete(timer);

	RTC.INTCTRL &= ~RTC_OVF_bm;

	timer->absolute_time = scheduler_make_absolute(timeout);

	// We only have to start the timer at head if the insert was at the head
	if (scheduler_sorted_insert(timer)) {
		scheduler_start_timer_at_head();
	} else {
		if (scheduler_is_running)
			RTC.INTCTRL |= RTC_OVF_bm;
	}
}

// NOTE: assumes the callback completes before the next timer tick
ISR(RTC_CNT_vect)
{
	timer_struct_t *next                    = scheduler_list_head->next;
	scheduler_absolute_time_of_last_timeout = scheduler_list_head->absolute_time;
	scheduler_last_timer_load               = 0;

	if (scheduler_list_head != &scheduler_dummy)
		scheduler_enqueue_callback(scheduler_list_head);

	// Remove expired timer for the list now (it is always the one at the head)
	scheduler_list_head = next;

	scheduler_start_timer_at_head();

	RTC.INTFLAGS = RTC_OVF_bm;
}

// These methods are for calculating the elapsed time in stopwatch mode.
// scheduler_timeout_start_timer will start a
// timer with (maximum range)/2. You cannot time more than
// this and the timer will stop after this time elapses
void scheduler_timeout_start_timer(timer_struct_t *timer)
{
	absolutetime_t i = -1;
	scheduler_timeout_create(timer, i >> 1);
}

// This funciton stops the "stopwatch" and returns the elapsed time.
absolutetime_t scheduler_timeout_stop_timer(timer_struct_t *timer)
{
	absolutetime_t now = scheduler_make_absolute(0); // Do this as fast as possible for accuracy
	absolutetime_t i   = -1;
	i >>= 1;

	scheduler_timeout_delete(timer);

	absolutetime_t diff = timer->absolute_time - now;

	// This calculates the (max range)/2 minus (remaining time) which = elapsed time
	return (i - diff);
}
