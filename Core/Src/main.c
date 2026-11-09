/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "max7219.h"
#include <stdlib.h>
#include "stm32l4xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define ADC_CENTER_LOW   1500
#define ADC_CENTER_HIGH  2500

#define THRESHOLD_LOW    500
#define THRESHOLD_HIGH   3500
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

RNG_HandleTypeDef hrng;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
extern SPI_HandleTypeDef hspi1;

/*
 * do this for timing
	uint32_t start_time = tick;
	if ((tick - start_time) >= 1000) {

    	start_time = tick;
	}
 */
volatile uint32_t tick = 0;
uint32_t game_start_time = 0; // Add this line

typedef enum {
	STATE_MENU,
	STATE_PLAYING,
	STATE_GAME_OVER
} GameState;

GameState current_state = STATE_MENU;

uint8_t myMatrix[8][8] = {
		{1, 0, 1, 0, 0, 1, 0, 1},
		{0, 1, 0, 1, 1, 0, 1, 0},
		{1, 0, 1, 0, 0, 1, 0, 1},
		{0, 1, 0, 1, 1, 0, 1, 0},
		{0, 1, 0, 1, 1, 0, 1, 0},
		{1, 0, 1, 0, 0, 1, 0, 1},
		{0, 1, 0, 1, 1, 0, 1, 0},
		{1, 0, 1, 0, 0, 1, 0, 1}
};

uint8_t solutionMatrix[8][8] = {};

uint8_t current_row = 0;
uint8_t current_col = 0;

bool axis_locked_x = false;
bool axis_locked_y = false;
bool button_locked = false;

uint8_t last_score = 0;

// A 3x5 pixel font for numbers 0-9
const uint8_t font3x5[10][5] = {
		{0b111, 0b101, 0b101, 0b101, 0b111}, // 0
		{0b010, 0b110, 0b010, 0b010, 0b111}, // 1
		{0b111, 0b001, 0b111, 0b100, 0b111}, // 2
		{0b111, 0b001, 0b111, 0b001, 0b111}, // 3
		{0b101, 0b101, 0b111, 0b001, 0b001}, // 4
		{0b111, 0b100, 0b111, 0b001, 0b111}, // 5
		{0b111, 0b100, 0b111, 0b101, 0b111}, // 6
		{0b111, 0b001, 0b010, 0b010, 0b010}, // 7
		{0b111, 0b101, 0b111, 0b101, 0b111}, // 8
		{0b111, 0b101, 0b111, 0b001, 0b111}  // 9
};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_RNG_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void custom_delay(uint32_t delay_ms) {
	uint32_t start_time = tick;
	while ((tick - start_time) < delay_ms) {
		// Block and wait
	}
}

void Display_Matix(uint8_t matrix[8][8])
{
	for (uint8_t row = 0; row < 8; row++)
	{
		uint8_t rowByte = 0x00;

		for (uint8_t col = 0; col < 8; col++)
		{
			if (matrix[7 - col][7 - row] == 1)
			{
				rowByte |= (1 << (7 - col));
			}
		}
		MAX7219_Digit(0, row, rowByte);
	}
}

void show_Solution(void) {
	Display_Matix(solutionMatrix);
	custom_delay(1500);

	for (int8_t i = 5; i >= 0; i--) {
		MAX7219_Intensity(0, i);
		custom_delay(1000);
	}

	MAX7219_Intensity(0, 0x05);
}

void Randomize_pattern(uint8_t matrix[8][8]){
    uint32_t true_rand_seed = 0;
    for (uint8_t r = 0; r < 8; r++)
    {
        for (uint8_t c = 0; c < 8; c++)
        {
            if (HAL_RNG_GenerateRandomNumber(&hrng, &true_rand_seed) == HAL_OK) {
                matrix[r][c] = true_rand_seed % 2;
            } else {
                matrix[r][c] = HAL_GetTick() % 2;
            }
        }
    }
}

void draw_score(uint8_t score) {
	uint8_t tempMatrix[8][8] = {0};

	uint8_t tens = score / 10;
	uint8_t ones = score % 10;

	for (uint8_t r = 0; r < 5; r++) {
		tempMatrix[r + 1][7] = (font3x5[tens][r] >> 2) & 1;
		tempMatrix[r + 1][6] = (font3x5[tens][r] >> 1) & 1;
		tempMatrix[r + 1][5] = (font3x5[tens][r] >> 0) & 1;

		tempMatrix[r + 1][3] = (font3x5[ones][r] >> 2) & 1;
		tempMatrix[r + 1][2] = (font3x5[ones][r] >> 1) & 1;
		tempMatrix[r + 1][1] = (font3x5[ones][r] >> 0) & 1;
	}

	Display_Matix(tempMatrix);
}

void animate_score_reveal(uint8_t final_score) {
	for (uint8_t display_score = 0; display_score <= final_score; display_score++) {
		draw_score(display_score);

		uint32_t delay_ms = 100;
		uint8_t distance = final_score - display_score;

		if (distance == 3) {
			delay_ms = 300;
		} else if (distance == 2) {
			delay_ms = 600;
		} else if (distance == 1) {
			delay_ms = 1000;
		} else if (distance == 0) {
			delay_ms = 0;
		}

		if (delay_ms > 0) {
			HAL_Delay(delay_ms);
		}
	}
}

void render_game(void) {
	uint8_t tempMatrix[8][8];

	for (uint8_t r = 0; r < 8; r++) {
		for (uint8_t c = 0; c < 8; c++) {
			tempMatrix[r][c] = myMatrix[r][c];
		}
	}

	bool blink_state = ((tick % 500) < 250);

	if (blink_state) {
		tempMatrix[current_row][current_col] ^= 1;
	}

	Display_Matix(tempMatrix);
}

void setup_Game(){
	for (uint8_t r = 0; r < 8; r++) {
		for (uint8_t c = 0; c < 8; c++) {
			myMatrix[r][c] = 0;
		}
	}
	Randomize_pattern(solutionMatrix);

}

void toggle_Circle(uint8_t row, uint8_t col){
	myMatrix[row][col] = (myMatrix[row][col] + 1) % 2;
}

void button_select(void) {

	switch (current_state) {
	case STATE_MENU:
		setup_Game();
		show_Solution();

		current_state = STATE_PLAYING;
		game_start_time = tick;
		break;

	case STATE_PLAYING:
		toggle_Circle(current_row, current_col);
		break;

	case STATE_GAME_OVER:
		// Clears the screen and returns to the menu
		for(uint8_t i = 0; i < 8; i++) {
			MAX7219_Digit(0, i, 0xFF);
			HAL_Delay(400);
			MAX7219_Digit(0, i, 0x00);
		}

		animate_score_reveal(last_score);

		current_state = STATE_MENU;
		break;
	}
}

void move_up(void) {
	switch (current_state) {
	case STATE_MENU:
		// Do nothing
		break;

	case STATE_PLAYING:
		// Move player cursor up on the 8x8 matrix
		if(current_row > 0){
			current_row--;
		}
		break;

	case STATE_GAME_OVER:
		// Do nothing
		break;
	}
}

void move_down(void) {
	switch (current_state) {
	case STATE_MENU:
		// Do nothing
		break;

	case STATE_PLAYING:
		// Move player cursor down on the 8x8 matrix
		if(current_row < 7){
			current_row++;
		}
		break;

	case STATE_GAME_OVER:
		// Do nothing
		break;
	}
}

void move_left(void) {
	switch (current_state) {
	case STATE_MENU:
		// Do nothing
		break;

	case STATE_PLAYING:
		// Move player cursor left on the 8x8 matrix
		if(current_col > 0){
			current_col--;
		}
		break;

	case STATE_GAME_OVER:
		// Do nothing
		break;
	}
}

void move_right(void) {
	switch (current_state) {
	case STATE_MENU:
		// Do nothing
		break;

	case STATE_PLAYING:
		// Move player cursor right on the 8x8 matrix
		if(current_col < 7){
			current_col++;
		}
		break;

	case STATE_GAME_OVER:
		// Do nothing
		break;
	}
}

void process_joystick(uint16_t adc_x, uint16_t adc_y, bool button_pressed) {

	// X Axis (Left / Right) Inverted
	if (adc_x < THRESHOLD_LOW && !axis_locked_x) {
		move_right();
		axis_locked_x = true;
	}
	else if (adc_x > THRESHOLD_HIGH && !axis_locked_x) {
		move_left();
		axis_locked_x = true;
	}
	// Unlock axis when the stick returns to the center deadzone
	else if (adc_x > ADC_CENTER_LOW && adc_x < ADC_CENTER_HIGH) {
		axis_locked_x = false;
	}

	// Y Axis (Up / Down) Inverted
	if (adc_y < THRESHOLD_LOW && !axis_locked_y) {
		move_down();
		axis_locked_y = true;
	}
	else if (adc_y > THRESHOLD_HIGH && !axis_locked_y) {
		move_up();
		axis_locked_y = true;
	}
	else if (adc_y > ADC_CENTER_LOW && adc_y < ADC_CENTER_HIGH) {
		axis_locked_y = false;
	}

	// Button (Click)
	if (button_pressed && !button_locked) {
		button_select();
		button_locked = true;
	}
	else if (!button_pressed) {
		button_locked = false;
	}
}

void check_Pattern(){
	int8_t temp_score = 0;
	int8_t temp_wrong = 0;

	for (uint8_t r = 0; r < 8; r++)
	{
		for (uint8_t c = 0; c < 8; c++)
		{
			if(myMatrix[r][c] == 1 && solutionMatrix[r][c] == 1){
				temp_score++;
				myMatrix[r][c] = 1;
			} else if(myMatrix[r][c] == 1 && solutionMatrix[r][c] == 0){
				temp_wrong++;
				myMatrix[r][c] = 0;
			} else {
				myMatrix[r][c] = 0;
			}
		}
	}

	temp_score = temp_score - (temp_wrong/2);

	if (temp_score < 0) {
		last_score = 0;
	} else {
		last_score = (uint8_t)temp_score;
	}

	HAL_Delay(5000);
}

void clear_Screen(){
	for(uint8_t i = 0; i < 8; i++) {
		MAX7219_Digit(0, i, 0xFF);
		HAL_Delay(500);
		MAX7219_Digit(0, i, 0x00);
	}
}



void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		//1 tick = 1 millisecond
		tick++;
	}
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
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_RNG_Init();
  /* USER CODE BEGIN 2 */

	MAX7219_Init(&hspi1, MTRX_CS_GPIO_Port, MTRX_CS_Pin);
	MAX7219_ShutDown(0, 1);    	// Wake up chip from sleep (1 = normal operation)
	MAX7219_Decode(0, 0x00);   	// Set to raw matrix mode (Disable 7-segment decoding)
	MAX7219_ScanLimit(0, 0x07);	// Tell chip to drive all 8 rows
	MAX7219_Intensity(0, 0x05);	// Set brightness level (0x00 min to 0x0F max)
	MAX7219_Test(0, 0);

	clear_Screen();

	HAL_TIM_Base_Start_IT(&htim2);

	uint32_t last_input_time = 0;
	uint32_t last_render_time = 0;

	// how long you have to answer in ms.
	uint32_t game_timeout_time = 30000;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		// INPUT POLLING (Every 50ms)
		if ((tick - last_input_time) >= 50) {
			last_input_time = tick;

			HAL_ADC_Start(&hadc1);
			HAL_ADC_PollForConversion(&hadc1, 10);
			uint16_t adc_y = HAL_ADC_GetValue(&hadc1);

			HAL_ADC_PollForConversion(&hadc1, 10);
			uint16_t adc_x = HAL_ADC_GetValue(&hadc1);
			HAL_ADC_Stop(&hadc1);

			bool button_pressed = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_2) == GPIO_PIN_RESET);
			process_joystick(adc_x, adc_y, button_pressed);
		}

		// TIMEOUT CHECK (dependent on game_timeout_time)
		if (current_state == STATE_PLAYING) {
			if ((tick - game_start_time) >= game_timeout_time) {
				current_state = STATE_GAME_OVER;

				check_Pattern();

				Display_Matix(myMatrix);
			}
		}

		// DISPLAY RENDERING (Every 20ms / 50Hz)
		if ((tick - last_render_time) >= 20) {
			last_render_time = tick;

			if (current_state == STATE_PLAYING) {
				render_game();
			} else if (current_state == STATE_MENU) {
				draw_score(last_score);
			}
		}

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
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RNG|RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_PLLSAI1;
  PeriphClkInit.RngClockSelection = RCC_RNGCLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_HSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 8;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV4;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK|RCC_PLLSAI1_ADC1CLK;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.NbrOfConversion = 2;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.OversamplingMode = DISABLE;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED;
  sConfig.OffsetNumber = ADC_OFFSET_NONE;
  sConfig.Offset = 0;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief RNG Initialization Function
  * @param None
  * @retval None
  */
static void MX_RNG_Init(void)
{

  /* USER CODE BEGIN RNG_Init 0 */

  /* USER CODE END RNG_Init 0 */

  /* USER CODE BEGIN RNG_Init 1 */

  /* USER CODE END RNG_Init 1 */
  hrng.Instance = RNG;
  if (HAL_RNG_Init(&hrng) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RNG_Init 2 */

  /* USER CODE END RNG_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 79;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, MTRX_CS_Pin|SMPS_EN_Pin|SMPS_V1_Pin|SMPS_SW_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD4_GPIO_Port, LD4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SWITCH_BUTTON_Pin */
  GPIO_InitStruct.Pin = SWITCH_BUTTON_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(SWITCH_BUTTON_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : MTRX_CS_Pin SMPS_EN_Pin SMPS_V1_Pin SMPS_SW_Pin */
  GPIO_InitStruct.Pin = MTRX_CS_Pin|SMPS_EN_Pin|SMPS_V1_Pin|SMPS_SW_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : SMPS_PG_Pin */
  GPIO_InitStruct.Pin = SMPS_PG_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(SMPS_PG_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD4_Pin */
  GPIO_InitStruct.Pin = LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD4_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
