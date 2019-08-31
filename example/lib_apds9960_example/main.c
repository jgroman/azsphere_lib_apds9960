
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include "applibs_versions.h"   // API struct versions to use for applibs APIs
#include <applibs/log.h>
#include <applibs/gpio.h>
#include <applibs/i2c.h>

// Import project hardware abstraction from project property 
// "Target Hardware Definition Directory"
#include <hw/project_hardware.h>

// Using a single-thread event loop pattern based on Epoll and timerfd
#include "epoll_timerfd_utilities.h"

#include "lib_apds9960.h"

/*******************************************************************************
* Forward declarations of private functions
*******************************************************************************/

/**
 * @brief Application termination handler.
 *
 * Signal handler for termination requests. This handler must be
 * async-signal-safe.
 *
 * @param signal_number
 *
 */
static void
termination_handler(int signal_number);

/**
 * @brief Initialize signal handlers.
 *
 * Set up SIGTERM termination handler.
 *
 * @return 0 on success, -1 otherwise.
 */
static int
init_handlers(void);

/** 
 * @brief Initialize peripherals.
 *
 * Initialize all peripherals used by this project.
 *
 * @return 0 on success, -1 otherwise.
 */
static int
init_peripherals(I2C_InterfaceId isu_id);

/**
 *
 */
static void
close_peripherals_and_handlers(void);

static void
apds9960_interrupt_handler(void);

static void
apds9960_int_timer_event_handler(EventData *event_data);

/*******************************************************************************
* Global variables
*******************************************************************************/

// Termination state flag
static volatile sig_atomic_t gb_is_termination_requested = false;
static int i2c_fd = -1;
static int epoll_fd = -1;
static apds9960_t *p_apds;

static int apds9960_int_poll_timer_fd = -1;
static int apds9960_int_gpio_fd = -1;
static GPIO_Value_Type apds9960_int_state = GPIO_Value_High;
static EventData apds9960_int_event_data = {
    .eventHandler = &apds9960_int_timer_event_handler
};


/*******************************************************************************
* Function definitions
*******************************************************************************/

int 
main(int argc, char *argv[])
{
    Log_Debug("\n*** Starting ***\n");

    gb_is_termination_requested = false;

    // Initialize handlers
    if (init_handlers() != 0)
    {
        gb_is_termination_requested = true;
    }

    // Initialize peripherals
    if (!gb_is_termination_requested)
    {
        if (init_peripherals(PROJECT_ISU2_I2C) != 0)
        {
            gb_is_termination_requested = true;
        }
    }

    // Main program
    if (!gb_is_termination_requested)
    {
        /*
        Log_Debug("ALS enable\n");
        apds9960_als_enable(p_apds, false);

        Log_Debug("Proximity enable\n");
        apds9960_proximity_enable(p_apds, false);
        */

        Log_Debug("Gesture enable\n");
        apds9960_gesture_enable(p_apds, true);

        Log_Debug("Entering main loop\n");

        uint8_t counter = 0;
        struct timespec sleep_time;
        //uint16_t clear_value;
        //uint16_t red_value;
        //uint16_t green_value;
        //uint16_t blue_value;

        //uint8_t prox_value;

        sleep_time.tv_nsec = 0;
        sleep_time.tv_sec = 2;

        // Main program loop
        while (!gb_is_termination_requested)
        {
            /*
            apds9960_als_read_clear(p_apds, &clear_value);
            apds9960_als_read_red(p_apds, &red_value);
            apds9960_als_read_green(p_apds, &green_value);
            apds9960_als_read_blue(p_apds, &blue_value);
            Log_Debug("ALS: clear 0x%04X, red 0x%04X, green 0x%04X, blue 0x%04X \n", clear_value, red_value, green_value, blue_value);

            apds9960_proximity_read(p_apds, &prox_value);
            Log_Debug("Proximity: 0x%02X\n", prox_value);
            */

            // Handle poll timers
            if (WaitForEventAndCallHandler(epoll_fd) != 0)
            {
                gb_is_termination_requested = true;
            }

            nanosleep(&sleep_time, NULL);

            counter++;
            if (counter == 20)
            {
                gb_is_termination_requested = true;
            }
        }

        Log_Debug("Leaving main loop\n");

        /*
        Log_Debug("ALS disable\n");
        apds9960_als_disable(p_apds);

        Log_Debug("Proximity disable\n");
        apds9960_proximity_disable(p_apds);
        */

        Log_Debug("Gesture disable\n");
        apds9960_gesture_disable(p_apds);


    }

    close_peripherals_and_handlers();

    Log_Debug("*** Terminating ***\n");
    return 0;
}

/*******************************************************************************
* Private function definitions
*******************************************************************************/

static void
termination_handler(int signal_number)
{
    gb_is_termination_requested = true;
}

static int
init_handlers(void)
{
    Log_Debug("Init Handlers\n");

    int result = -1;

    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = termination_handler;
    result = sigaction(SIGTERM, &action, NULL);
    if (result != 0) {
        Log_Debug("ERROR: %s - sigaction: errno=%d (%s)\n",
            __FUNCTION__, errno, strerror(errno));
    }

    epoll_fd = CreateEpollFd();
    if (epoll_fd < 0) {
        result = -1;
    }

    return result;
}

static int
init_peripherals(I2C_InterfaceId isu_id)
{
    int result = -1;

    // Initialize I2C
    Log_Debug("Init I2C\n");
    i2c_fd = I2CMaster_Open(isu_id);
    if (i2c_fd < 0) {
        Log_Debug("ERROR: I2CMaster_Open: errno=%d (%s)\n",
            errno, strerror(errno));
    }
    else
    {
        result = I2CMaster_SetBusSpeed(i2c_fd, I2C_BUS_SPEED_STANDARD);
        if (result != 0) {
            Log_Debug("ERROR: I2CMaster_SetBusSpeed: errno=%d (%s)\n",
                errno, strerror(errno));
        }
        else
        {
            result = I2CMaster_SetTimeout(i2c_fd, 100);
            if (result != 0) {
                Log_Debug("ERROR: I2CMaster_SetTimeout: errno=%d (%s)\n",
                    errno, strerror(errno));
            }
        }
    }

    // Initialize APDS9960 board
    if (result != -1)
    {
        Log_Debug("Init APDS9960\n");
        p_apds = apds9960_open(i2c_fd, APDS9960_I2C_ADDRESS);

        if (!p_apds)
        {
            Log_Debug("ERROR> Could not initialize APDS9960.\n");
            result = -1;
        }

    }

    // Set development kit Socket 1 & 2 INT pin as Input
    if (result != -1)
    {
        Log_Debug("Opening PROJECT_SOCKET12_INT as input.\n");
        apds9960_int_gpio_fd = GPIO_OpenAsInput(PROJECT_SOCKET12_INT);
        if (apds9960_int_gpio_fd < 0) {
            Log_Debug("ERROR: Could not open GPIO: %s (%d).\n",
                strerror(errno), errno);
            result = -1;
        }
    }

    // Create timer for apds9960 interrupt signal check
    if (result != -1)
    {
        struct timespec apds9960_int_check_period = { 0, 250000000 };
        apds9960_int_poll_timer_fd = CreateTimerFdAndAddToEpoll(epoll_fd,
            &apds9960_int_check_period, &apds9960_int_event_data, EPOLLIN);
        if (apds9960_int_poll_timer_fd < 0)
        {
            Log_Debug("ERROR: Could not create interrupt poll timer: %s (%d).\n",
                strerror(errno), errno);
            result = -1;
        }
    }

    return result;
}

static void
close_peripherals_and_handlers(void)
{
    // Close APDS9960 sensor
    Log_Debug("Close APDS9960\n");
    if (p_apds)
    {
        apds9960_close(p_apds);
    }

    // Close I2C
    CloseFdAndPrintError(i2c_fd, "I2C");

    // Close APDS9960 interrupt GPIO fd
    CloseFdAndPrintError(apds9960_int_gpio_fd, "APDS9960 INT GPIO");

    // Close Epoll fd
    CloseFdAndPrintError(epoll_fd, "Epoll");

}

static void
apds9960_interrupt_handler(void)
{
    Log_Debug("APDS interrupt\n");

    bool b_gesture_valid;
    apds9960_gesture_is_valid(p_apds, &b_gesture_valid);

    if (b_gesture_valid)
    {
        int gesture = apds9960_gesture_read(p_apds);

        Log_Debug("Gesture %d \n", gesture);

        switch (gesture)
        {
        case GESTURE_DIR_UP:
            Log_Debug("Up\n");
            break;
        case GESTURE_DIR_DOWN:
            Log_Debug("Down\n");
            break;
        case GESTURE_DIR_LEFT:
            Log_Debug("Left\n");
            break;
        case GESTURE_DIR_RIGHT:
            Log_Debug("Right\n");
            break;
        case GESTURE_DIR_FAR:
            Log_Debug("Far\n");
            break;
        case GESTURE_DIR_NEAR:
            Log_Debug("Near\n");
            break;

        default:
            Log_Debug("Unknown\n");
            break;
        }
    }


}

static void
apds9960_int_timer_event_handler(EventData *event_data)
{
    // Consume timer event
    if (ConsumeTimerFdEvent(apds9960_int_poll_timer_fd) != 0) {
        gb_is_termination_requested = true;
        return;
    }

    // Check for interrupt signal state change
    GPIO_Value_Type new_apds9960_int_state;

    int result = GPIO_GetValue(apds9960_int_gpio_fd, &new_apds9960_int_state);
    if (result != 0) {
        Log_Debug("ERROR: Could not read apds9960 interrupt GPIO: %s (%d).\n",
            strerror(errno), errno);
        gb_is_termination_requested = true;
        return;
    }

    if (new_apds9960_int_state != apds9960_int_state)
    {
        if (new_apds9960_int_state == GPIO_Value_Low)
        {
            // apds9960 /INT pin is asserted. New measurement is available.
            apds9960_interrupt_handler();
        }
        apds9960_int_state = new_apds9960_int_state;
    }
}

/* [] END OF FILE */
