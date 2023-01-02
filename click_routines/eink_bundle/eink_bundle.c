// DOM-IGNORE-BEGIN
/*******************************************************************************
 * Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
 *
 * Subject to your compliance with these terms, you may use Microchip software
 * and any derivatives exclusively with Microchip products. It is your
 * responsibility to comply with third party license terms applicable to your
 * use of third party software (including open source software) that may
 * accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
 * EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
 * WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
 * INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
 * WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
 * BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
 * FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
 * ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *******************************************************************************/
// DOM-IGNORE-END

#include <math.h>
#include <string.h>
#include "definitions.h"
#include "click_routines/eink_bundle/eink_bundle.h"
#include "click_routines/eink_bundle/eink_bundle_image.h"
#include "click_routines/eink_bundle/eink_bundle_font.h"
#include "app.h"

uint8_t frame[4000];

char demo_text[ 8 ] = {'E', '-', 'P', 'a', 'p', 'e', 'r', 0};
char demo_text1[ 8 ] = {'D', 'i', 's', 'p', 'l', 'a', 'y', 0};
char demo_text2[ 9 ] = {'2', '.', '1', '3', 'i', 'n', 'c', 'h', 0};


// 70-bytes

//const unsigned char lut_data[] ={
//0xA0,0x90,0x50,0x00,0x00,0x00,0x00,
//0x50,0x90,0xA0,0x00,0x00,0x00,0x00,
//0xA0,0x90,0x50,0x00,0x00,0x00,0x00,
//0x50,0x90,0xA0,0x00,0x00,0x00,0x00,
//0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//0x0F,0x0F,0x0F,0x0F,0x02,0x10,0x10,
//0x0A,0x0A,0x03,0x08,0x08,0x09,0x43,
//0x07,0x00,0x00,0x00,0x00,0x00,0x00,
//0x00,0x00,0x00,0x00,0x00,0x00,0x00,
//0x00,0x00,0x00,0x00,0x00,0x00,0x00
//};

const unsigned char lut_data[] = {
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, // LUT0: BB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00, // LUT1: BW:     VS 0 ~7
    0x80, 0x60, 0x40, 0x00, 0x00, 0x00, 0x00, // LUT2: WB:     VS 0 ~7
    0x10, 0x60, 0x20, 0x00, 0x00, 0x00, 0x00, // LUT3: WW:     VS 0 ~7
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // LUT4: VCOM:   VS 0 ~7

    0x03, 0x03, 0x00, 0x00, 0x02, // TP0 A~D RP0
    0x09, 0x09, 0x00, 0x00, 0x02, // TP1 A~D RP1
    0x03, 0x03, 0x00, 0x00, 0x02, // TP2 A~D RP2
    0x00, 0x00, 0x00, 0x00, 0x00, // TP3 A~D RP3
    0x00, 0x00, 0x00, 0x00, 0x00, // TP4 A~D RP4
    0x00, 0x00, 0x00, 0x00, 0x00, // TP5 A~D RP5
    0x00, 0x00, 0x00, 0x00, 0x00, // TP6 A~D RP6    
};



/* Text Variables */
static const uint8_t* _font;
static uint16_t _font_color;
static uint8_t _font_orientation;
static uint16_t _font_first_char;
static uint16_t _font_last_char;
static uint16_t _font_height;
static uint16_t x_cord;
static uint16_t y_cord;
static uint16_t n_char;


/******************************************************************************
 * Private Function Prototypes
 *******************************************************************************/

static void hal_eink_write(uint8_t* buffer, uint16_t count);
static void eink_reset(void);
static void char_wr(uint16_t ch_idx);
static void frame_px(uint16_t x, uint16_t y, uint8_t font_col);

/******************************************************************************
 * Private Function Definitions
 *******************************************************************************/
static void eink_reset(void) {
    CLICK_EINK_BUNDLE_RST_Set();
    vTaskDelay(1 / portTICK_PERIOD_MS);
    CLICK_EINK_BUNDLE_RST_Clear();
    vTaskDelay(2 / portTICK_PERIOD_MS);
    CLICK_EINK_BUNDLE_RST_Set();
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

void eink_sendCmd(uint8_t cmd) {
    while (CLICK_EINK_BUNDLE_BSY_Get() == 1);
    CLICK_EINK_BUNDLE_DC_Clear();
    hal_eink_write(&cmd, 1);
    while (CLICK_EINK_BUNDLE_BSY_Get() == 1);
}

void eink_sendData(uint8_t data) {
    while (CLICK_EINK_BUNDLE_BSY_Get() == 1);
    CLICK_EINK_BUNDLE_DC_Set();
    hal_eink_write(&data, 1);
    while (CLICK_EINK_BUNDLE_BSY_Get() == 1);
}

static void hal_eink_write(uint8_t* buffer, uint16_t count) {
    DRV_SPI_WriteTransferAdd(appData.handle, buffer, count, &appData.transferHandle);
    while (DRV_SPI_TransferStatusGet(appData.transferHandle) == DRV_SPI_TRANSFER_EVENT_PENDING);
    vTaskDelay(1 / portTICK_PERIOD_MS);
}

void eink_init(void) {
    CLICK_EINK_BUNDLE_DC_Set();
    CLICK_EINK_BUNDLE_CS_Set();
    eink_reset();
    eink_sendCmd(EPAPER_SW_RESET);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    eink_powerup();
    vTaskDelay(500 / portTICK_PERIOD_MS);
}

void eink_powerup(void) {
    uint16_t i = 0;
    // Do not enter into deep sleep mode
    eink_sendCmd(EPAPER_DEEP_SLEEP_MODE);
    eink_sendData(0x00);

    // Set Analog block control
    eink_sendCmd(EPAPER_SET_ANALOG_BLOCK_CONTROL);
    eink_sendData(ANALOG_BLOCK_CONTROL_VALUE);
    // Set Digital block control
    eink_sendCmd(EPAPER_SET_DIGITAL_BLOCK_CONTROL);
    eink_sendData(DIGITAL_BLOCK_CONTROL_VALUE);

    // Set Display size and driver output control
    eink_sendCmd(EPAPER_DRIVER_OUTPUT_CONTROL);
    eink_sendData((EINK213_DISPLAY_HEIGHT - 1) & 0xFF);
    eink_sendData(((EINK213_DISPLAY_HEIGHT - 1) >> 8) & 0xFF);
    eink_sendData(0x00);

    // Set Dummy line period
    eink_sendCmd(EPAPER_SET_DUMMY_LINE_PERIOD);
    eink_sendData(DUMMY_LINE_PERIOD_VALUE);

    // Set Gate line width
    eink_sendCmd(EPAPER_SET_GATE_LINE_WIDTH);
    eink_sendData(GATE_LINE_WDITH_VALUE);

    // Set Data Entry sequence
    eink_sendCmd(EPAPER_DATA_ENTRY_MODE);
    eink_sendData(0x01); // y decrement, x increment

    // Set RAM X-address start/end position
    eink_sendCmd(EPAPER_SET_RAM_X);
    eink_sendData(0x00); // x-start
    eink_sendData((EINK213_DISPLAY_WIDTH / 8) - 1); // x-end data=0x0F

    // Set RAM Y-address start/end position
    eink_sendCmd(EPAPER_SET_RAM_Y);
    eink_sendData(EINK213_DISPLAY_HEIGHT - 1); // y-start data=0xF9
    eink_sendData((EINK213_DISPLAY_HEIGHT - 1) >> 8); // y-start data=0x00
    eink_sendData(0x00); // y-end
    eink_sendData(0x00); // y-end

    //Set RAM x, y counter
    eink_setRAMaddress(0, EINK213_DISPLAY_HEIGHT - 1);

    // Set pattern display
    eink_sendCmd(EPAPER_DISPLAY_UPDATE_1);
    eink_sendData(0x03);

    // Set VCOM voltage
    eink_sendCmd(EPAPER_WRITE_VCOM_REGISTER);
    eink_sendData(VCOM_REGISTER_VALUE);
    // Set Border waveform
    eink_sendCmd(EPAPER_BORDER_WAVEFORM);
    eink_sendData(BORDER_WAVEFORM_VALUE);


    //    // Set Gate voltage
    //    eink_sendCmd(EPAPER_GATE_DRIVING_VOLTAGE_CONTROL);
    //    eink_sendData(0x15); //data=0x15

    //    // Set Source voltage
    //    eink_sendCmd(EPAPER_SOURCE_DRIVING_VOLTAGE_CONTROL);
    //    eink_sendData(0x41); //data=0x41
    //    eink_sendData(0xA8); //data=0xA8
    //    eink_sendData(0x32); //data=0x32

    // Write LUT data
    eink_sendCmd(EPAPER_WRITE_LUT_REGISTER);
    for (i = 0; i < LUT_DATA_SIZE; i++) {
        eink_sendData(lut_data[i]);
    }

    //    // Set RAM address
    //    eink_setRAMaddress(0, EINK213_DISPLAY_HEIGHT - 1);

}

void eink_powerDown(void) {
    eink_sendCmd(EPAPER_DEEP_SLEEP_MODE);
    eink_sendData(0x01);
}

void eink_resetCounter(void) {
    eink_setRAMaddress(0, EINK213_DISPLAY_HEIGHT - 1);
}

void eink_setRAMaddress(uint16_t x, uint16_t y) {
    if (x >= EINK213_DISPLAY_WIDTH)
        x = EINK213_DISPLAY_WIDTH - 1;
    if (y >= EINK213_DISPLAY_HEIGHT)
        y = EINK213_DISPLAY_HEIGHT - 1;

    // Set RAM X-Address counter
    eink_sendCmd(EPAPER_SET_RAM_X_ADDRESS_COUNTER);
    eink_sendData(x & 0xFF);

    // Set RAM Y-Address counter
    eink_sendCmd(EPAPER_SET_RAM_Y_ADDRESS_COUNTER);
    eink_sendData(y & 0xFF);
    eink_sendData(y >> 8);
}

void eink_fill_screen(uint8_t color) {
    uint16_t i;
    eink_sendCmd(EPAPER_WRITE_RAM);
    for (i = 0; i < EINK213_DISPLAY_RESOLUTIONS; i++) {
        eink_sendData(color);
    }
    eink_updateFull();
}

void eink_frame_bg(uint8_t bgcolor) {
    uint16_t i;
    for (i = 0; i < EINK213_DISPLAY_RESOLUTIONS; i++) {
        frame[i] = bgcolor;
    }
}

void eink_updateFull(void) {
    eink_sendCmd(EPAPER_DISPLAY_UPDATE_2);
    eink_sendData(DISPLAY_UPDATE_SEQUENCE_MODE_1);
    eink_sendCmd(EPAPER_MASTER_ACTIVATION);
    eink_sendCmd(EPAPER_NO_OPERATION);
}

void eink_updatePartial(void) {
    eink_sendCmd(EPAPER_DISPLAY_UPDATE_2);
    eink_sendData(DISPLAY_UPDATE_SEQUENCE_MODE_2);
    eink_sendCmd(EPAPER_MASTER_ACTIVATION);
    eink_sendCmd(EPAPER_NO_OPERATION);
}

void eink_display_image(const uint8_t* image_buffer, uint16_t img_x_start, uint16_t img_x_end, uint16_t img_y_start, uint16_t img_y_end) {
    uint16_t cnt_x;
    uint16_t cnt_y;
    uint16_t pos;
    uint16_t x_start = img_x_start;
    uint16_t y_start = img_y_start;
    uint16_t x_end = img_x_end - 1;
    uint16_t y_end = img_y_end - 1;

    eink_sendCmd(EPAPER_SET_RAM_X); //set memory area
    eink_sendData((x_start >> 3) & 0xFF);
    eink_sendData((x_end >> 3) & 0xFF);

    eink_sendCmd(EPAPER_SET_RAM_Y);
    eink_sendData(y_start & 0xFF);
    eink_sendData((y_start >> 8) & 0xFF);
    eink_sendData(y_end & 0xFF);
    eink_sendData((y_end >> 8) & 0xFF);


    for (cnt_y = img_y_start; cnt_y < img_y_end; cnt_y++) {
        eink_sendCmd(EPAPER_SET_RAM_X_ADDRESS_COUNTER); //set memory point
        eink_sendData(((img_x_start >> 3) & 0xFF));

        eink_sendCmd(EPAPER_SET_RAM_Y_ADDRESS_COUNTER);
        eink_sendData((cnt_y & 0xFF));
        eink_sendData(((cnt_y >> 8) & 0xFF));

        eink_sendCmd(EPAPER_WRITE_RAM);

        for (cnt_x = 0; cnt_x < 16; cnt_x++) {
            pos = cnt_x + (cnt_y * 16);
            eink_sendData(image_buffer[ pos ]);
        }
    }
    //    eink_updateFull();
    eink_updatePartial();
}

void eink_set_font(const uint8_t *font, uint16_t color, uint8_t orientation) {
    _font = font;
    _font_first_char = font[2] + (font[3] << 8);
    _font_last_char = font[4] + (font[5] << 8);
    _font_height = font[6];
    _font_color = color;
    _font_orientation = orientation;
}

void eink_text(char *text, uint8_t n, uint16_t text_x_start, uint16_t text_x_end, uint16_t text_y_start, uint16_t text_y_end) {

    uint16_t cnt;
    uint16_t cnt_x;
    uint16_t cnt_y;
    uint16_t pos;

    x_cord = text_x_start;
    y_cord = text_y_start;
    n_char = n;

    if ((x_cord >= EINK213_DISPLAY_WIDTH) || (y_cord >= EINK213_DISPLAY_HEIGHT)) {
        return;
    }

    uint16_t x_start = x_cord;
    uint16_t y_start = y_cord;
    uint16_t x_end = text_x_end - 1; //EINK213_DISPLAY_WIDTH - 1;
    uint16_t y_end = text_y_end - 1; //EINK213_DISPLAY_HEIGHT - 1;

    for (cnt = 0; cnt < n_char; cnt++) {
        char_wr(text[cnt]);
    }

    eink_sendCmd(EPAPER_SET_RAM_X); //set memory area
    eink_sendData((x_start >> 3) & 0xFF);
    eink_sendData((x_end >> 3) & 0xFF);

    eink_sendCmd(EPAPER_SET_RAM_Y);
    eink_sendData(y_start & 0xFF);
    eink_sendData((y_start >> 8) & 0xFF);
    eink_sendData(y_end & 0xFF);
    eink_sendData((y_end >> 8) & 0xFF);

    for (cnt_y = text_y_start; cnt_y < text_y_end; cnt_y++) {
        eink_sendCmd(EPAPER_SET_RAM_X_ADDRESS_COUNTER); //set memory point
        eink_sendData(((0 >> 3) & 0xFF));

        eink_sendCmd(EPAPER_SET_RAM_Y_ADDRESS_COUNTER);
        eink_sendData((cnt_y & 0xFF));
        eink_sendData(((cnt_y >> 8) & 0xFF));

        eink_sendCmd(EPAPER_WRITE_RAM);
        for (cnt_x = 0; cnt_x < 16; cnt_x++) {
            pos = cnt_x + (cnt_y * 16);
            eink_sendData(frame[ pos ]);
        }
    }

    vTaskDelay(2 / portTICK_PERIOD_MS);
    //    eink_updateFull();
    eink_updatePartial();
}

static void char_wr(uint16_t ch_idx) {
    uint8_t ch_width = 0;
    uint8_t x_cnt;
    uint8_t y_cnt;
    uint16_t x = 0;
    uint16_t y = 0;
    uint16_t tmp;
    uint8_t temp = 0;
    uint8_t mask = 0;
    uint32_t offset;
    const uint8_t *ch_table;
    const uint8_t *ch_bitmap;

    if (ch_idx < _font_first_char) {
        return;
    }
    if (ch_idx > _font_last_char) {
        return;
    }

    offset = 0;
    tmp = (ch_idx - _font_first_char) << 2;
    ch_table = (const uint8_t*) (_font + (8 + tmp));
    ch_width = *ch_table;

    offset = (uint32_t) ch_table[ 1 ] + ((uint32_t) ch_table [ 2 ] << 8) + ((uint32_t) ch_table[ 3 ] << 16);

    ch_bitmap = _font + offset;

    if ((_font_orientation == FO_HORIZONTAL) ||
            (_font_orientation == FO_VERTICAL_COLUMN)) {
        y = y_cord;
        for (y_cnt = 0; y_cnt < _font_height; y_cnt++) {
            x = x_cord;
            mask = 0;
            for (x_cnt = 0; x_cnt < ch_width; x_cnt++) {
                if (!mask) {
                    temp = *ch_bitmap++;
                    mask = 0x01;
                }

                if (temp & mask) {
                    frame_px(x, y, _font_color);
                }

                x++;
                mask <<= 1;
            }
            y++;
        }

        if (_font_orientation == FO_HORIZONTAL) {
            x_cord = x + 1;
        } else {
            y_cord = y;
        }
    } else {
        y = x_cord;

        for (y_cnt = 0; y_cnt < _font_height; y_cnt++) {
            x = y_cord;
            mask = 0;

            for (x_cnt = 0; x_cnt < ch_width; x_cnt++) {
                if (mask == 0) {
                    temp = *ch_bitmap++;
                    mask = 0x01;
                }

                if (temp & mask) {
                    frame_px(x, y, _font_color);
                }

                x--;
                mask <<= 1;
            }
            y++;
        }
        y_cord = x - 1;
    }
}

static void frame_px(uint16_t x, uint16_t y, uint8_t font_col) {
    uint16_t off;
    uint16_t pos;

    pos = (y * (EINK213_DISPLAY_WIDTH / 8)) + (x / 4);
    off = fabs(3 - (x % 4)) * 2;

    frame[ pos ] &= ~(0x03 << off);
    frame[ pos ] |= ((font_col & 0x03) << off);
}

void eink_example(void) {

    eink_init();
    //    eink_resetCounter();


    eink_fill_screen(EINK_COLOR_WHITE);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    //    eink_fill_screen(EINK_COLOR_BLACK);
    //    vTaskDelay(500 / portTICK_PERIOD_MS);
    //    eink_fill_screen(EINK_COLOR_LIGHT_GREY);
    //    vTaskDelay(500 / portTICK_PERIOD_MS);
    //    eink_fill_screen(EINK_COLOR_DARK_GREY);
    //    vTaskDelay(50 / portTICK_PERIOD_MS);

    eink_display_image(mchp_128x80_black, 0, 128, 0, 80);
    vTaskDelay(10 / portTICK_PERIOD_MS);

    //    eink_display_image(mchp_128x80_white,0,128,0,80);
    //    vTaskDelay(1000 / portTICK_PERIOD_MS);

    eink_frame_bg(EINK_COLOR_WHITE);
    eink_set_font(guiFont_Tahoma_10_Regular, EINK_COLOR_BLACK, FO_HORIZONTAL);
    eink_text(demo_text, 7, 10, 128, 195, 215);
    eink_text(demo_text1, 7, 10, 128, 215, 235);

    eink_set_font(guiFont_Tahoma_8_Regular, EINK_COLOR_DARK_GREY, FO_HORIZONTAL);
    eink_text(demo_text2, 8, 10, 128, 235, 250);


    //    eink_frame_bg(EINK_COLOR_WHITE);
    //    eink_set_font(guiFont_Tahoma_10_Regular, EINK_COLOR_BLACK, FO_HORIZONTAL);
    //    eink_text(demo_text2, 8, 10, 128, 195, 215);
    //    eink_text(demo_text, 7, 10, 128, 235, 250);

}

void e_paper_initialize(void) {
    APP_Msg_T appMsg;
    appMsg.msgId = APP_MSG_E_PAPER_EVT;
    appMsg.msgData[0] = APP_E_PAPER_INIT;
    OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
}

void e_paper_print(uint8_t display, uint8_t line, uint8_t *data) {
    APP_Msg_T appMsg;
    uint8_t dataLen, dataEndPos;

    if ((line == 2) || (line == 1)) {
        dataLen = strlen((char *) data);

        if (dataLen > TXT_STR2_MAX_LEN) {
            dataLen = TXT_STR2_MAX_LEN;
            appMsg.msgData[TXT_STR2_MAX_LEN + DISP_DATA_OFFSET] = '\0';
        }

        dataEndPos = dataLen + DISP_DATA_OFFSET;
        appMsg.msgId = APP_MSG_E_PAPER_EVT;
        appMsg.msgData[0] = APP_E_PAPER_PRINT;
        appMsg.msgData[1] = display;
        appMsg.msgData[2] = line;
        appMsg.msgData[3] = dataLen;
        strcpy((char *) &appMsg.msgData[DISP_DATA_OFFSET + 1], (char *) data);

        if (line == 2) {
            SERCOM0_USART_Write((uint8_t *) "DATA RECEIVED: ", 15);
            SERCOM0_USART_Write((uint8_t *) data, dataLen);
            SERCOM0_USART_Write((uint8_t *) "\r\n", 2);
        }

        if (dataLen > (TXT_STR2_MAX_LEN / 2)) {
            for (uint8_t i = dataEndPos; i > 3; i--) {
                if (appMsg.msgData[i] == 32) //check for space character
                {
                    appMsg.msgData[i] = 10; //insert new line
                    break;
                }
            }
        }
        OSAL_QUEUE_Send(&appData.appQueue, &appMsg, 0);
    }
}

void APP_E_PAPER_Handler(uint8_t *msgData) {
    APP_E_PAPER_T e_paper_event = msgData[0];

    switch (e_paper_event) {
        case APP_E_PAPER_INIT:
        {
            appData.handle = DRV_SPI_Open(DRV_SPI_INDEX_0, DRV_IO_INTENT_EXCLUSIVE);

            DRV_SPI_TRANSFER_SETUP setup;

            setup.baudRateInHz = 20000000;
            setup.clockPhase = DRV_SPI_CLOCK_PHASE_VALID_LEADING_EDGE;
            setup.clockPolarity = DRV_SPI_CLOCK_POLARITY_IDLE_LOW;
            setup.dataBits = DRV_SPI_DATA_BITS_8;
            setup.chipSelect = SYS_PORT_PIN_RA9; //GPIO_PIN_RA9
            setup.csPolarity = DRV_SPI_CS_POLARITY_ACTIVE_LOW;

            DRV_SPI_TransferSetup(appData.handle, &setup);
            vTaskDelay(100 / portTICK_PERIOD_MS);

            eink_example(); //ROSHAN
        }
            break;
        case APP_E_PAPER_PRINT:
        {
            if (msgData[1] == 0) {
                if (msgData[2] == 1) {
                    eink_frame_bg(EINK_COLOR_WHITE);
                    eink_set_font(guiFont_Tahoma_6_Regular, EINK_COLOR_BLACK, FO_HORIZONTAL);
                    eink_text((char *) &msgData[4], msgData[3], 0, 128, 81, 95);
                } else if (msgData[2] == 2) {
                    eink_frame_bg(EINK_COLOR_WHITE);
                    eink_set_font(guiFont_Tahoma_8_Regular, EINK_COLOR_BLACK, FO_HORIZONTAL);
                    eink_text((char *) &msgData[4], msgData[3], 0, 128, 96, 110);
                }
            }
        }
        default:
        {
            /* TODO: Handle error in application's state machine. */
            break;
        }
    }
}


/*************** END OF FUNCTIONS ***************************************************************************/