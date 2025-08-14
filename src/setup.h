//
// setup.h
// Setup mode functions for PiGFX configuration
//
// PiGFX is a bare metal kernel for the Raspberry Pi
// that implements a basic ANSI terminal emulator with
// the additional support of some primitive graphics functions.
// Copyright (C) 2025

#ifndef SETUP_H_
#define SETUP_H_

// Setup mode functions
extern void setup_mode_enter(void);
extern void setup_mode_exit(void);
extern unsigned char setup_mode_is_active(void);
extern void setup_mode_draw(void);
extern void setup_mode_handle_key(unsigned short key);

#endif
