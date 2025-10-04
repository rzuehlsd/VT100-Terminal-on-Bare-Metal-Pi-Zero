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

// Enter setup/configuration mode and initialize menu state
extern void setup_mode_enter(void);

// Exit setup mode and restore previous terminal state
extern void setup_mode_exit(void);

// Check if setup mode is currently active
extern unsigned char setup_mode_is_active(void);

// Render the setup mode user interface
extern void setup_mode_draw(void);

// Handle keyboard input while in setup mode
extern void setup_mode_handle_key(unsigned short key);

#endif
