// pwm.c - simple software PWM at fixed 800 Hz on GPIO12
// Uses the existing timer system (one-shot handlers) to toggle GPIO12
// Caveat: Requires timer_poll() to be called frequently (already in main loop)

#include "gpio.h"
#include "timer.h"
#include "utils.h"
#include "pwm.h"

#define PWM_GPIO 12
#define PWM_FREQ_HZ 800u
#define PWM_PERIOD_US (1000000u / PWM_FREQ_HZ) // 1250 us

// Internal state
static volatile unsigned pwm_timer_period = 0; // 800 Hz scheduler
static volatile unsigned pwm_timer_off = 0;    // one-shot off timer within period
static volatile uint32_t pwm_end_time = 0; // absolute time in usec, 0 = infinite
static volatile uint8_t pwm_active = 0;
static volatile uint8_t pwm_duty = 0; // 0..100

static void pwm_period_handler(unsigned hnd, void* pParam, void* pContext);
static void pwm_off_handler(unsigned hnd, void* pParam, void* pContext);

static void pwm_period_handler(unsigned hnd, void* pParam, void* pContext)
{
    (void)pParam; (void)pContext; (void)hnd;
    pwm_timer_period = 0;

    // Stop condition
    if (pwm_end_time && (int)(time_microsec() - pwm_end_time) >= 0) {
        if (pwm_timer_off) { remove_timer(pwm_timer_off); pwm_timer_off = 0; }
        gpio_set(PWM_GPIO, 0);
        pwm_active = 0;
        return;
    }

    unsigned on_us = (PWM_PERIOD_US * (unsigned)pwm_duty) / 100u;

    if (on_us == 0) {
        // 0% duty: keep low
        gpio_set(PWM_GPIO, 0);
    } else if (on_us >= PWM_PERIOD_US) {
        // 100% duty: keep high
        gpio_set(PWM_GPIO, 1);
    } else {
        // Normal case: turn high now, schedule off after on_us
        gpio_set(PWM_GPIO, 1);
        unsigned hz_off = (on_us > 0) ? (1000000u / on_us) : 0;
        if (hz_off == 0) hz_off = 1;
        pwm_timer_off = attach_timer_handler(hz_off, pwm_off_handler, 0, 0);
    }

    // Schedule next period tick at 800 Hz
    pwm_timer_period = attach_timer_handler(PWM_FREQ_HZ, pwm_period_handler, 0, 0);
}

static void pwm_off_handler(unsigned hnd, void* pParam, void* pContext)
{
    (void)pParam; (void)pContext; (void)hnd;
    pwm_timer_off = 0;
    gpio_set(PWM_GPIO, 0);
}

void pwm800_start(uint8_t duty_percent, uint32_t duration_ms)
{
    if (duty_percent > 100) duty_percent = 100;

    // Configure GPIO12 as output low
    gpio_select(PWM_GPIO, GPIO_OUTPUT);
    gpio_setpull(PWM_GPIO, GPIO_PULL_OFF);
    gpio_set(PWM_GPIO, 0);

    // If already active, stop first
    pwm800_stop();

    pwm_duty = duty_percent;
    pwm_active = 1;

    if (duration_ms == 0) {
        pwm_end_time = 0; // infinite
    } else {
        pwm_end_time = time_microsec() + duration_ms * 1000u;
    }

    // Kick off immediately with a period tick
    pwm_period_handler(0, 0, 0);
}

void pwm800_stop(void)
{
    if (pwm_timer_period) { remove_timer(pwm_timer_period); pwm_timer_period = 0; }
    if (pwm_timer_off) { remove_timer(pwm_timer_off); pwm_timer_off = 0; }
    gpio_set(PWM_GPIO, 0);
    pwm_active = 0;
    pwm_end_time = 0;
}

int pwm800_is_active(void)
{
    return pwm_active;
}
