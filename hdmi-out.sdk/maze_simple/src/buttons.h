#pragma once
#ifndef BUTTONS_H
#define BUTTONS_H

#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"

#define INTC_DEVICE_ID           XPAR_PS7_SCUGIC_0_DEVICE_ID
#define INTC_BTNS_INTERRUPT_ID   XPAR_FABRIC_AXI_GPIO_0_IP2INTC_IRPT_INTR
#define BTN_DEVICE_ID            XPAR_AXI_GPIO_0_DEVICE_ID
#define CH1_INT_MASK             XGPIO_IR_CH1_MASK


typedef struct ButtonState {
	XScuGic intc;
	XGpio btn_gpio;
	int btn_val;
} button_state_t;



void button_interrupt_handler(void *callback_ref);
int initialize_buttons(button_state_t *button_state);
int button_setup_interrupts(button_state_t *button_state);



#endif /* BUTTONS_H */
