// pwm.h - simple software PWM at fixed 800 Hz on GPIO12
// Inputs: duty in percent (0..100), duration_ms (0 = continuous)
// API: pwm800_start(duty_percent, duration_ms); pwm800_stop(); pwm800_is_active();

#ifndef _PIGFX_PWM_H_
#define _PIGFX_PWM_H_

#include <stdint.h>

void pwm800_start(uint8_t duty_percent, uint32_t duration_ms);
void pwm800_stop(void);
int  pwm800_is_active(void);

#endif // _PIGFX_PWM_H_
