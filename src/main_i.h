#ifndef _MAIN_INCLUDE_H
#define _MAIN_INCLUDE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/irq.h"
#include "hardware/timer.h"
#include "bmc.pio.h"
#include "4b5b.h"
#include "pd_frame.h"
#include "pd_message.h"
#include "crc32.h"
#include "bmc_rx.h"
#include "rp2040_bmc.h"
#include "cli/cli.h"

#endif
