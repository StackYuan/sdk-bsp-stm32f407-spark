/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2023-5-10      ShiHao       first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>

#define DBG_TAG "main"
#define DBG_LVL DBG_LOG
#include <rtdbg.h>

//#include <drv_lcd.h>
#include <rttlogo.h>
#include "lcd_ili9341.h"

/* 配置 LED 灯引脚 */
#define PIN_LED_B              GET_PIN(F, 11)      // PF11 :  LED_B        --> LED
#define PIN_LED_R              GET_PIN(F, 12)      // PF12 :  LED_R        --> LED

extern int lcd_spi_test(void);


#define TOUCH_THREAD_PRIORITY     25
#define TOUCH_THREAD_STACK_SIZE   1024
#define TOUCH_THREAD_TIMESLICE    10

static rt_sem_t xpt2046_sem = RT_NULL;
static rt_device_t dev = RT_NULL;
static struct rt_touch_info info;
static struct rt_touch_data *read_data;

static rt_err_t touch_rx_callback(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(xpt2046_sem);
    rt_device_control(dev, RT_TOUCH_CTRL_DISABLE_INT, RT_NULL);
    return 0;
}

static void touch_thread_entry(void *parameter)
{
    rt_uint16_t point_x, point_y;

    rt_device_control(dev, RT_TOUCH_CTRL_GET_INFO, &info);

    read_data = (struct rt_touch_data *) rt_malloc(sizeof(struct rt_touch_data) * info.point_num);

    while (1)
    {
        rt_sem_take(xpt2046_sem, RT_WAITING_FOREVER);

        if (rt_device_read(dev, 0, read_data, info.point_num) == info.point_num)
        {
            point_x = read_data[0].x_coordinate;
            point_y = read_data[0].y_coordinate;
#ifdef BSP_USING_LVGL
            switch (read_data[0].event)
            {
            case TOUCH_EVENT_UP:
                post_up_event(point_x, point_y);
                break;
            case TOUCH_EVENT_DOWN:
                post_down_event(point_x, point_y);
                break;
            case TOUCH_EVENT_MOVE:
                post_motion_event(point_x, point_y);
                break;
            default:
                break;
            }
#else
            if (read_data[0].event == RT_TOUCH_EVENT_DOWN || read_data[0].event == RT_TOUCH_EVENT_MOVE)
            {
                LOG_D("%d %d %d %d %d\n", read_data[0].track_id, point_x,
                        point_y, read_data[0].timestamp, read_data[0].width);
            }
#endif
        }
        rt_device_control(dev, RT_TOUCH_CTRL_ENABLE_INT, RT_NULL);
    }
}

static int touch_bg_init(const char *name, rt_uint16_t x, rt_uint16_t y)
{
    void *id;
    rt_thread_t tid = RT_NULL;

    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("can't find device:%s\n", name);
        return -1;
    }

    if (rt_device_open(dev, RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        rt_kprintf("open device failed!");
        return -1;
    }

    xpt2046_sem = rt_sem_create("xtpsem", 0, RT_IPC_FLAG_FIFO);
    if (xpt2046_sem == RT_NULL)
    {
        rt_kprintf("create dynamic semaphore failed.\n");
        return -1;
    }
    //@TODO: read id

    rt_device_set_rx_indicate(dev, touch_rx_callback);
    tid = rt_thread_create("touch", touch_thread_entry, RT_NULL, TOUCH_THREAD_STACK_SIZE, TOUCH_THREAD_PRIORITY, TOUCH_THREAD_TIMESLICE);
    if (tid != RT_NULL)
        rt_thread_startup(tid);

    return 0;
}

int main(void)
{

    spi_lcd_init(20);
    touch_bg_init("xpt0", RT_NULL, RT_NULL);
//    while(1)
//    {
//        lcd_spi_test();
//    }
//    lcd_clear(WHITE);

//    /* show RT-Thread logo */
//    lcd_show_image(0, 0, 240, 69, image_rttlogo);

//    /* set the background color and foreground color */
//    lcd_set_color(WHITE, BLACK);

//    /* show some string on lcd */
//    lcd_show_string(10, 69, 16, "Hello, RT-Thread!");
//    lcd_show_string(10, 69 + 16, 24, "RT-Thread");
//    lcd_show_string(10, 69 + 16 + 24, 32, "RT-Thread");

//    /* draw a line on lcd */
//    lcd_draw_line(0, 69 + 16 + 24 + 32, 240, 69 + 16 + 24 + 32);

//    /* draw a concentric circles */
//    lcd_draw_point(120, 194);
//    for (int i = 0; i < 46; i += 4)
//    {
//        lcd_draw_circle(120, 194, i);
//    }


    return 0;
}

