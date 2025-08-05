/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "iwdg.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* Common definitions */
#define TRUE	1
#define FALSE	0

/* Injector periods */
#define NUMBER_OF_PERIODS	6
#define DEFAULT_PERIOD_IDX	2	//1Hz

const uint16_t auto_reload_periods[NUMBER_OF_PERIODS] = {50000, 20000, 10000, 5000, 2000, 1000};	//0.2Hz, 0.5Hz, 1Hz, 2Hz, 5Hz, 10Hz

/* Keyboard definitions */
#define KEYBOARD_TIME	200	//ms

#define PERIOD_KEY_IS_PRESSED			(!(LL_GPIO_IsInputPinSet(PERIOD_BTN_GPIO_Port, PERIOD_BTN_Pin)))
#define ENABLE_KEY_IS_PRESSED			(!(LL_GPIO_IsInputPinSet(ON_BTN_GPIO_Port, ON_BTN_Pin)))
#define ALL_KEYS_ARE_RELEASED			(LL_GPIO_IsInputPinSet(PERIOD_BTN_GPIO_Port, PERIOD_BTN_Pin) && LL_GPIO_IsInputPinSet(ON_BTN_GPIO_Port, ON_BTN_Pin))

#define CLEAR_KBD_TIMER					LL_TIM_SetCounter(TIM14, 0)

/* LEDs definitions */
#define ENABLE_LED_ON					LL_GPIO_ResetOutputPin(ON_LED_GPIO_Port, ON_LED_Pin)
#define ENABLE_LED_OFF					LL_GPIO_SetOutputPin(ON_LED_GPIO_Port, ON_LED_Pin)

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void Change_period(uint16_t period, uint8_t pulses_enabled)
{
	/* Disable outputs and timer counter */
	LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);
	LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH3);
	LL_TIM_DisableCounter(TIM1);

	/* Reset counter */
	LL_TIM_SetCounter(TIM1, 0);

	/* Change period */
	LL_TIM_SetAutoReload(TIM1, period-1);

	/* Enable LED */
	LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);

	/* Enable INJ pulses back only if flag is set */
	if(pulses_enabled == TRUE)
	{
		LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH2);
	}

	/* Enable counter back again */
	LL_TIM_EnableCounter(TIM1);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  LL_APB1_GRP2_EnableClock(LL_APB1_GRP2_PERIPH_SYSCFG);
  LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

  /* SysTick_IRQn interrupt configuration */
  NVIC_SetPriority(SysTick_IRQn, 3);

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_TIM14_Init();
  MX_IWDG_Init();
  /* USER CODE BEGIN 2 */

	uint8_t keyboard_locked = TRUE;

	uint8_t injector_pulses_enabled = FALSE;
	uint8_t current_period = DEFAULT_PERIOD_IDX;

	/* Set default period */
	LL_TIM_SetAutoReload(TIM1, auto_reload_periods[DEFAULT_PERIOD_IDX]-1);		//1Hz

	/* Enable outputs */
	LL_TIM_EnableAllOutputs(TIM1);

	/* Enable counter */
	LL_TIM_EnableCounter(TIM1);

	/* Enable pulse LED, disable INJ pulses */
	LL_TIM_CC_EnableChannel(TIM1, LL_TIM_CHANNEL_CH3);
	LL_TIM_CC_DisableChannel(TIM1, LL_TIM_CHANNEL_CH2);

	ENABLE_LED_OFF;

	/* Init keyboard timer */
	LL_TIM_EnableCounter(TIM14);

	CLEAR_KBD_TIMER;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		if(keyboard_locked == FALSE)
		{
			if(PERIOD_KEY_IS_PRESSED)
			{
				/* Switch to next period */
				current_period++;
				if(current_period >= NUMBER_OF_PERIODS)
				{
					current_period = 0;
				}

				/* Update settings */
				Change_period(auto_reload_periods[current_period], injector_pulses_enabled);

				/* Debounce keys */
				keyboard_locked = TRUE;
				CLEAR_KBD_TIMER;
			}
			else if(ENABLE_KEY_IS_PRESSED)
			{
				/* Toggle enable flag */
				if(injector_pulses_enabled == TRUE)
				{
					ENABLE_LED_OFF;

					injector_pulses_enabled = FALSE;
				}
				else
				{
					ENABLE_LED_ON;

					injector_pulses_enabled = TRUE;
				}

				/* Update settings */
				Change_period(auto_reload_periods[current_period], injector_pulses_enabled);

				/* Debounce keys */
				keyboard_locked = TRUE;
				CLEAR_KBD_TIMER;
			}
		}
		else if(keyboard_locked == TRUE)
		{
			if(LL_TIM_GetCounter(TIM14) > KEYBOARD_TIME)
			{
				/* Unlock keyboard if all keys are released */
				if(ALL_KEYS_ARE_RELEASED)
				{
					keyboard_locked = FALSE;
				}
				else
				{
					/* Reset timer */
					CLEAR_KBD_TIMER;
				}
			}
		}

		/* Handle WDT */
		LL_IWDG_ReloadCounter(IWDG);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_0);
  while(LL_FLASH_GetLatency() != LL_FLASH_LATENCY_0)
  {
  }
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_LSI_Enable();

   /* Wait till LSI is ready */
  while(LL_RCC_LSI_IsReady() != 1)
  {

  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_1);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_HSE);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_HSE)
  {

  }
  LL_Init1msTick(8000000);
  LL_SetSystemCoreClock(8000000);
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
