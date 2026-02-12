/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define TIM2_CH1_DC1_PWM_Pin GPIO_PIN_0
#define TIM2_CH1_DC1_PWM_GPIO_Port GPIOA
#define TIM2_CH2_DC2_PWM_Pin GPIO_PIN_1
#define TIM2_CH2_DC2_PWM_GPIO_Port GPIOA
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define DC1_DIR_Pin GPIO_PIN_4
#define DC1_DIR_GPIO_Port GPIOA
#define LD2_Pin GPIO_PIN_5
#define LD2_GPIO_Port GPIOA
#define LSW_5_Pin GPIO_PIN_6
#define LSW_5_GPIO_Port GPIOA
#define TIM3_CH2_RE1_B_Pin GPIO_PIN_7
#define TIM3_CH2_RE1_B_GPIO_Port GPIOA
#define LSW_4_Pin GPIO_PIN_4
#define LSW_4_GPIO_Port GPIOC
#define DC2_DIR_Pin GPIO_PIN_0
#define DC2_DIR_GPIO_Port GPIOB
#define TIM2_CH3_SERVO1_PWM_Pin GPIO_PIN_10
#define TIM2_CH3_SERVO1_PWM_GPIO_Port GPIOB
#define LSW_3_Pin GPIO_PIN_12
#define LSW_3_GPIO_Port GPIOB
#define TIM3_CH1_RE1_A_Pin GPIO_PIN_6
#define TIM3_CH1_RE1_A_GPIO_Port GPIOC
#define TIM1_CH1_RE3_B_Pin GPIO_PIN_8
#define TIM1_CH1_RE3_B_GPIO_Port GPIOA
#define TIM1_CH2_RE3_A_Pin GPIO_PIN_9
#define TIM1_CH2_RE3_A_GPIO_Port GPIOA
#define LSW_2_Pin GPIO_PIN_11
#define LSW_2_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define LSW_1_Pin GPIO_PIN_15
#define LSW_1_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
#define TIM4_CH1_RE2_A_Pin GPIO_PIN_6
#define TIM4_CH1_RE2_A_GPIO_Port GPIOB
#define TIM4_CH2_RE2_B_Pin GPIO_PIN_7
#define TIM4_CH2_RE2_B_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
