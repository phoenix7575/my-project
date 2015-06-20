#include <avr/interrupt.h>
#include <util/delay.h>

#include "matrix.h"
#include "fonts.h"
#include "mtimer.h"
#include "display.h"
#include "ds18x20.h"
#include "rtc.h"
#include "alarm.h"

void hwInit(void)
{
	_delay_ms(100);
	sei();
	ds18x20SearchDevices();

	displayInit();
	matrixInit();

	mTimerInit();
	matrixScrollAndADCInit();

	alarmInit();
	rtcStopEditTime();

	return;
}

int main(void)
{
	uint8_t cmd;
	uint8_t dispMode = MODE_MAIN;
	int8_t direction = PARAM_UP;

	uint32_t mask = MASK_ALL;

	hwInit();

	while(1) {
		/* Measure temperature with TEMP_POLL_INTERVAL period */
		if (getTempStartTimer() == 0) {
			setTempStartTimer(TEMP_POLL_INTERVAL);
			ds18x20Process();
		}

		if (dispMode != MODE_BRIGHTNESS)
			checkAlarmAndBrightness();

		/* Get command from buttons */
		cmd = getBtnCmd();

		/* Beep when button is pressed */
		if (cmd != CMD_EMPTY) {
			if (cmd < CMD_BTN_1_LONG)
				startBeeper(80);
			else
				startBeeper(160);
		}

		/* Stop scrolling when button is pressed */
		if (cmd != CMD_EMPTY)
			matrixHwScroll(MATRIX_SCROLL_STOP);

		/* Handle command */
		switch (cmd) {
		case CMD_BTN_1:
			direction = PARAM_UP;
			switch (dispMode) {
			case MODE_EDIT_TIME:
				rtcNextEditParam();
				break;
			case MODE_ALARM:
				dispMode = MODE_EDIT_ALARM;
				alarmNextEditParam();
				break;
			case MODE_EDIT_ALARM:
				if (alarm.eam == ALARM_SUN) {
					dispMode = MODE_ALARM;
					showAlarm(MASK_ALL);
				} else {
					alarmNextEditParam();
				}
				break;
			}
			break;
		case CMD_BTN_2:
			direction = PARAM_UP;
			switch (dispMode) {
			case MODE_MAIN:
				scroll(SCROLL_DATE);
				break;
			case MODE_EDIT_TIME:
				rtcChangeTime(PARAM_UP);
				break;
			case MODE_EDIT_ALARM:
				alarmChange(PARAM_UP);
				break;
			case MODE_BRIGHTNESS:
				changeBrightness(PARAM_UP);
				break;
			}
			break;
		case CMD_BTN_3:
			direction = PARAM_DOWN;
			switch (dispMode) {
			case MODE_MAIN:
				scroll(SCROLL_TEMP);
				break;
			case MODE_EDIT_TIME:
				rtcChangeTime(PARAM_DOWN);
				break;
			case MODE_EDIT_ALARM:
				alarmChange(PARAM_DOWN);
				break;
			case MODE_BRIGHTNESS:
				changeBrightness(PARAM_DOWN);
				break;
			}
			break;
		case CMD_BTN_1_LONG:
			switch (dispMode) {
			case MODE_EDIT_TIME:
				rtcStopEditTime();
				dispMode = MODE_MAIN;
				showTime(MASK_ALL);
				break;
			case MODE_MAIN:
				dispMode = MODE_EDIT_TIME;
				rtcNextEditParam();
			}
			break;
		case CMD_BTN_2_LONG:
			switch (dispMode) {
			case MODE_ALARM:
			case MODE_EDIT_ALARM:
				dispMode = MODE_MAIN;
				showTime(MASK_ALL);
				alarmSave();
				break;
			case MODE_MAIN:
				dispMode = MODE_ALARM;
				mask = MASK_ALL;
				break;
			}
			break;
		case CMD_BTN_3_LONG:
			switch (dispMode) {
			case MODE_BRIGHTNESS:
				dispMode = MODE_MAIN;
				saveMaxBrightness();
				showTime(MASK_ALL);
				break;
			case MODE_MAIN:
				dispMode = MODE_BRIGHTNESS;
				mask = MASK_ALL;
				break;
			}
			break;
		case CMD_BTN_1_2_LONG:
			displaySwitchHourZero();
			dispMode = MODE_MAIN;
			showTime(MASK_ALL);
			break;
		case CMD_BTN_2_3_LONG:
			displaySwitchBigNum();
			dispMode = MODE_MAIN;
			showTime(MASK_ALL);
			break;
		case CMD_BTN_1_2_3_LONG:
			matrixScreenRotate();
			break;
		}

		/* Show things */
		switch (dispMode) {
		case MODE_MAIN:
			showMainScreen();
			break;
		case MODE_EDIT_TIME:
			showTimeEdit(direction);
			break;
		case MODE_ALARM:
			showAlarm(mask);
			mask = MASK_NONE;
			break;
		case MODE_EDIT_ALARM:
			showAlarmEdit(direction);
			break;
		case MODE_BRIGHTNESS:
			showBrightness(direction, mask);
			mask = MASK_NONE;
			break;
		}
	}

	return 0;
}
