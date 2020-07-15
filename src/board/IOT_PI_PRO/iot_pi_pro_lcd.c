/*********************************************************************************************************
**
**                                北京翼辉信息技术有限公司
**
**                                  微型安全实时操作系统
**
**                                      MS-RTOS(TM)
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: iot_pi_pro_lcd.c
**
** 创   建   人: Yu.kangzhi
**
** 文件创建日期: 2020 年 04 月 07 日
**
** 描        述: IoT Pi Pro LCD 驱动
**
** 参 考 文  件: stm32h750b_discovery_lcd.c
*********************************************************************************************************/

#include "ms_config.h"
#include "ms_rtos.h"
#include "includes.h"
#include "iot_pi_pro.h"

/*********************************************************************************************************
 LCD Private Variables
*********************************************************************************************************/

static uint32_t Instance = 0;
static uint32_t ActiveLayer = 0;

static TIM_HandleTypeDef hlcd_tim;

/*********************************************************************************************************
 LCD Exported Variables
*********************************************************************************************************/

DMA2D_HandleTypeDef hlcd_dma2d;
LTDC_HandleTypeDef  hlcd_ltdc;
BSP_LCD_Ctx_t       Lcd_Ctx[LCD_INSTANCES_NBR];

/*********************************************************************************************************
 LCD Private FunctionPrototypes
*********************************************************************************************************/
static void LCD_InitSequence(void);
static void LCD_DeInitSequence(void);
static void LTDC_MspInit(LTDC_HandleTypeDef *hltdc);
static void LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc);
static void DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d);
static void DMA2D_MspDeInit(DMA2D_HandleTypeDef *hdma2d);
static void LL_FillBuffer(uint32_t *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t Color);
static void LL_ConvertLineToRGB(uint32_t *pSrc, uint32_t *pDst, uint32_t xSize, uint32_t ColorMode);
static void TIMx_PWM_MspInit(TIM_HandleTypeDef *htim);
static void TIMx_PWM_MspDeInit(TIM_HandleTypeDef *htim);
static void TIMx_PWM_DeInit(TIM_HandleTypeDef *htim);
static void TIMx_PWM_Init(TIM_HandleTypeDef *htim);

/*********************************************************************************************************
 LCD Private Macros
*********************************************************************************************************/
#define CONVERTRGB5652ARGB8888(Color)((((((((Color) >> (11U)) & 0x1FU) * 527U) + 23U) >> (6U)) << (16U)) |\
                                     (((((((Color) >> (5U)) & 0x3FU) * 259U) + 33U) >> (6U)) << (8U)) |\
                                     (((((Color) & 0x1FU) * 527U) + 23U) >> (6U)) | (0xFF000000U))

/*********************************************************************************************************
 LCD Frame Buffer Testing
*********************************************************************************************************/
#if LCD_TEST_FRAME_BUFFER_EN > 0

static void LCD_FB_Test(void)
{
    int i;
    uint16_t *frame_buffer = (uint16_t*)(LCD_TEST_FRAME_BUFFER_ADDR);

    //RED
    for (i = 0; i < BSP_CFG_LCD_WIDTH*BSP_CFG_LCD_HEIGHT; i++) {
        frame_buffer[i] = LCD_COLOR_RGB565_RED;
    }

    //GREEN
    for (i = 0; i < BSP_CFG_LCD_WIDTH*BSP_CFG_LCD_HEIGHT; i++) {
        frame_buffer[i] = LCD_COLOR_RGB565_GREEN;
    }

    //BLUE
    for (i = 0; i < BSP_CFG_LCD_WIDTH*BSP_CFG_LCD_HEIGHT; i++) {
        frame_buffer[i] = LCD_COLOR_RGB565_BLUE;
    }

    //WHITE
    for (i = 0; i < BSP_CFG_LCD_WIDTH*BSP_CFG_LCD_HEIGHT; i++) {
        frame_buffer[i] = LCD_COLOR_RGB565_WHITE;
    }

    //BLACK
    for (i = 0; i < BSP_CFG_LCD_WIDTH*BSP_CFG_LCD_HEIGHT; i++) {
        frame_buffer[i] = LCD_COLOR_RGB565_BLACK;
    }
}

#endif
/*********************************************************************************************************
 LCD Exported Functions
*********************************************************************************************************/

/**
  * @brief  Initializes the LCD in default mode.
  * @param  Instance    LCD Instance
  * @param  Orientation LCD_ORIENTATION_PORTRAIT or LCD_ORIENTATION_LANDSCAPE
  * @retval BSP status
  */
int32_t BSP_LCD_Init()
{
  int ret;
  uint32_t Orientation = LCD_ORIENTATION_LANDSCAPE;

#if LCD_TEST_FRAME_BUFFER_EN > 0

  if (BSP_LCD_InitEx(Orientation, LCD_PIXEL_FORMAT_RGB565, LCD_DEFAULT_WIDTH, LCD_DEFAULT_HEIGHT) != HAL_OK)
  {
      return BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
      BSP_LCD_LayerRgb565Init(0, LCD_LAYER_0_ADDRESS);
      BSP_LCD_SelectLayer(0);
      BSP_LCD_Clear(LCD_COLOR_WHITE);
      BSP_LCD_SetLayerVisible(0, ENABLE);
      BSP_LCD_DisplayOn();

      //test
      LCD_FB_Test();

      return BSP_ERROR_NONE;
  }
#else

  ret = BSP_LCD_InitEx(Orientation, LCD_PIXEL_FORMAT_RGB565, LCD_DEFAULT_WIDTH, LCD_DEFAULT_HEIGHT);

  BSP_LCD_Clear(LCD_COLOR_WHITE);

  return ret;

#endif
}

/**
  * @brief  Initializes the LCD.
  * @param  Instance    LCD Instance
  * @param  Orientation LCD_ORIENTATION_PORTRAIT or LCD_ORIENTATION_LANDSCAPE
  * @param  PixelFormat LCD_PIXEL_FORMAT_RGB565 or LCD_PIXEL_FORMAT_RGB888
  * @param  Width       Display width
  * @param  Height      Display height
  * @retval BSP status
  */
int32_t BSP_LCD_InitEx(uint32_t Orientation, uint32_t PixelFormat, uint32_t Width, uint32_t Height)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t ltdc_pixel_format;
  MX_LTDC_LayerConfig_t config;

  if((Orientation > LCD_ORIENTATION_LANDSCAPE) || (Instance >= LCD_INSTANCES_NBR) || \
     ((PixelFormat != LCD_PIXEL_FORMAT_RGB565) && (PixelFormat != LCD_PIXEL_FORMAT_ARGB8888)))
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(PixelFormat == LCD_PIXEL_FORMAT_RGB565)
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_RGB565;
      Lcd_Ctx[Instance].BppFactor = 2U;
    }
    else /* LCD_PIXEL_FORMAT_RGB888 */
    {
      ltdc_pixel_format = LTDC_PIXEL_FORMAT_ARGB8888;
      Lcd_Ctx[Instance].BppFactor = 4U;
    }

    /* Store pixel format, xsize and ysize information */
    Lcd_Ctx[Instance].PixelFormat = PixelFormat;
    Lcd_Ctx[Instance].XSize  = Width;
    Lcd_Ctx[Instance].YSize  = Height;

    /* Toggle Hardware Reset of the LCD using its XRES signal (active low) */
    BSP_LCD_Reset(Instance);

    /* Initialize LCD special pins GPIOs */
    LCD_InitSequence();

    /* Initializes peripherals instance value */
    hlcd_ltdc.Instance = LTDC;
    hlcd_dma2d.Instance = DMA2D;

    /* MSP initialization */
#if (USE_HAL_LTDC_REGISTER_CALLBACKS == 1)
    /* Register the LTDC MSP Callbacks */
    if(Lcd_Ctx[Instance].IsMspCallbacksValid == 0U)
    {
      if(BSP_LCD_RegisterDefaultMspCallbacks(0) != BSP_ERROR_NONE)
      {
        return BSP_ERROR_PERIPH_FAILURE;
      }
    }
#else
    LTDC_MspInit(&hlcd_ltdc);
#endif

    DMA2D_MspInit(&hlcd_dma2d);

    if(MX_LTDC_ClockConfig(&hlcd_ltdc) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
    if(MX_LTDC_Init(&hlcd_ltdc, Width, Height) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    }

    if(ret == BSP_ERROR_NONE)
    {
      /* Configure default LTDC Layer 0. This configuration can be override by calling
      BSP_LCD_ConfigLayer() at application level */
      config.X0          = 0;
      config.X1          = Width;
      config.Y0          = 0;
      config.Y1          = Height;
      config.PixelFormat = ltdc_pixel_format;
      config.Address     = LCD_LAYER_0_ADDRESS;
      if(MX_LTDC_ConfigLayer(&hlcd_ltdc, 0, &config) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }

      /* Initialize TIM in PWM mode to control brightness */
      //TIMx_PWM_Init(&hlcd_tim);

      /* By default the reload is activated and executed immediately */
      Lcd_Ctx[Instance].ReloadEnable = 1U;
    }
  }

  return ret;
}

/**
  * @brief  De-Initializes the LCD resources.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DeInit(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    LCD_DeInitSequence();

#if (USE_HAL_LTDC_REGISTER_CALLBACKS == 0)
    LTDC_MspDeInit(&hlcd_ltdc);
#endif /* (USE_HAL_LTDC_REGISTER_CALLBACKS == 0) */

    DMA2D_MspDeInit(&hlcd_dma2d);

    (void)HAL_LTDC_DeInit(&hlcd_ltdc);
    if(HAL_DMA2D_DeInit(&hlcd_dma2d) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      /* DeInit TIM PWM */
      //TIMx_PWM_DeInit(&hlcd_tim);

      Lcd_Ctx[Instance].IsMspCallbacksValid = 0;
    }
  }

  return ret;
}

/**
  * @brief  Clears the hole LCD.
  * @param  Color: Color of the background
  * @retval None
  */
void BSP_LCD_Clear(uint32_t Color)
{
    BSP_LCD_FillRect(0, 0, Lcd_Ctx[Instance].XSize, Lcd_Ctx[Instance].YSize, Color);
}

/**
  * @brief  Selects the LCD Layer.
  * @param  LayerIndex: Layer foreground or background
  * @retval None
  */
void BSP_LCD_SelectLayer(uint32_t LayerIndex)
{
    ActiveLayer = LayerIndex;
}

/**
  * @brief  Initializes the LCD layer in RGB565 format (16 bits per pixel).
  * @param  LayerIndex: Layer foreground or background
  * @param  FB_Address: Layer frame buffer
  * @retval None
  */
void BSP_LCD_LayerRgb565Init(uint16_t LayerIndex, uint32_t FB_Address)
{
    /*
     * BSP_LCD_LayerRgb565Init: MX_LTDC_ConfigLayer
     */
}

/**
  * @brief  BSP LCD Reset
  *         Hw reset the LCD DSI activating its XRES signal (active low for some time)
  *         and deactivating it later.
  * @param  Instance LCD Instance
  */
void BSP_LCD_Reset()
{
    /* There is no LCD reset pin, so do nothing here. */
}

/**
  * @brief  Initializes the LTDC.
  * @param  hltdc  LTDC handle
  * @param  Width  LTDC width
  * @param  Height LTDC height
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_LTDC_Init(LTDC_HandleTypeDef *hltdc, uint32_t Width, uint32_t Height)
{
  hltdc->Instance = LTDC;
  hltdc->Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc->Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc->Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc->Init.PCPolarity = LTDC_PCPOLARITY_IPC;

  hltdc->Init.HorizontalSync     = LCD_TIMING_HSYNC - 1U;
  hltdc->Init.VerticalSync       = LCD_TIMING_VSYNC - 1U;
  hltdc->Init.AccumulatedHBP     = LCD_TIMING_HSYNC + LCD_TIMING_HBP - 1U;
  hltdc->Init.AccumulatedVBP     = LCD_TIMING_VSYNC + LCD_TIMING_VBP - 1U;
  hltdc->Init.AccumulatedActiveW = LCD_TIMING_HSYNC + LCD_TIMING_HBP + Width - 1U;
  hltdc->Init.AccumulatedActiveH = LCD_TIMING_VSYNC + LCD_TIMING_VBP + Height - 1U;
  hltdc->Init.TotalWidth         = LCD_TIMING_HSYNC + LCD_TIMING_HBP + Width + LCD_TIMING_HFP - 1U;
  hltdc->Init.TotalHeigh         = LCD_TIMING_VSYNC + LCD_TIMING_VBP + Height + LCD_TIMING_VFP - 1U;

  hltdc->Init.Backcolor.Blue  = 0xFF;
  hltdc->Init.Backcolor.Green = 0xFF;
  hltdc->Init.Backcolor.Red   = 0xFF;

  return HAL_LTDC_Init(hltdc);
}

/**
  * @brief  MX LTDC layer configuration.
  * @param  hltdc      LTDC handle
  * @param  LayerIndex Layer 0 or 1
  * @param  Config     Layer configuration
  * @retval HAL status
  */
__weak HAL_StatusTypeDef MX_LTDC_ConfigLayer(LTDC_HandleTypeDef *hltdc, uint32_t LayerIndex, MX_LTDC_LayerConfig_t *Config)
{
  LTDC_LayerCfgTypeDef pLayerCfg;

  pLayerCfg.WindowX0 = Config->X0;
  pLayerCfg.WindowX1 = Config->X1;
  pLayerCfg.WindowY0 = Config->Y0;
  pLayerCfg.WindowY1 = Config->Y1;
  pLayerCfg.PixelFormat = Config->PixelFormat;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = Config->Address;
  pLayerCfg.ImageWidth = (Config->X1 - Config->X0);
  pLayerCfg.ImageHeight = (Config->Y1 - Config->Y0);

  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;

  return HAL_LTDC_ConfigLayer(hltdc, &pLayerCfg, LayerIndex);
}

/**
  * @brief  LTDC Clock Config for LCD DPI display.
  * @param  hltdc  LTDC Handle
  *         Being __weak it can be overwritten by the application
  * @retval HAL_status
  */
__weak HAL_StatusTypeDef MX_LTDC_ClockConfig(LTDC_HandleTypeDef *hltdc)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hltdc);

  RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

  /* RK043FN48H LCD clock configuration */
  /* LCD clock configuration */
  /* PLL3_VCO Input = HSE_VALUE/PLL3M = 5 Mhz */
  /* PLL3_VCO Output = PLL3_VCO Input * PLL3N = 800 Mhz */
  /* PLLLCDCLK = PLL3_VCO Output/PLL3R = 800/83 = 9.63 Mhz */
  /* LTDC clock frequency = PLLLCDCLK = 9.63 Mhz */
  PeriphClkInitStruct.PeriphClockSelection   = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLL3.PLL3M = 5;
  PeriphClkInitStruct.PLL3.PLL3N = 160;
  PeriphClkInitStruct.PLL3.PLL3P = 2;
  PeriphClkInitStruct.PLL3.PLL3Q = 2;
  PeriphClkInitStruct.PLL3.PLL3R = 88;
  PeriphClkInitStruct.PLL3.PLL3VCOSEL = 0;
  PeriphClkInitStruct.PLL3.PLL3FRACN = 0;

  return HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct);
}

#if (USE_HAL_LTDC_REGISTER_CALLBACKS == 1)
/**
  * @brief Default BSP LCD Msp Callbacks
  * @param Instance BSP LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_RegisterDefaultMspCallbacks (uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPINIT_CB_ID, LTDC_MspInit) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPDEINIT_CB_ID, LTDC_MspDeInit) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }

    Lcd_Ctx[Instance].IsMspCallbacksValid = 1;
  }

  return ret;
}

/**
  * @brief BSP LCD Msp Callback registering
  * @param Instance    LCD Instance
  * @param CallBacks   pointer to LCD MspInit/MspDeInit functions
  * @retval BSP status
  */
int32_t BSP_LCD_RegisterMspCallbacks (BSP_LCD_Cb_t *CallBacks)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPINIT_CB_ID, CallBacks->pMspLtdcInitCb) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
    else
    {
      if(HAL_LTDC_RegisterCallback(&hlcd_ltdc, HAL_LTDC_MSPDEINIT_CB_ID, CallBacks->pMspLtdcDeInitCb) != HAL_OK)
      {
        ret = BSP_ERROR_PERIPH_FAILURE;
      }
    }

    Lcd_Ctx[Instance].IsMspCallbacksValid = 1;
  }

  return ret;
}
#endif /*(USE_HAL_LTDC_REGISTER_CALLBACKS == 1) */

/**
  * @brief  LTDC layer configuration.
  * @param  Instance   LCD instance
  * @param  LayerIndex Layer 0 or 1
  * @param  Config     Layer configuration
  * @retval HAL status
  */
int32_t BSP_LCD_ConfigLayer(uint32_t LayerIndex, BSP_LCD_LayerConfig_t *Config)
{
  int32_t ret = BSP_ERROR_NONE;
  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if (MX_LTDC_ConfigLayer(&hlcd_ltdc, LayerIndex, Config) != HAL_OK)
    {
      ret = BSP_ERROR_PERIPH_FAILURE;
    }
  }
  return ret;
}

/**
  * @brief  Gets the LCD Active LCD Pixel Format.
  * @param  Instance    LCD Instance
  * @param  PixelFormat Active LCD Pixel Format
  * @retval BSP status
  */
int32_t BSP_LCD_GetPixelFormat(uint32_t *PixelFormat)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Only RGB565 format is supported */
    *PixelFormat = Lcd_Ctx[Instance].PixelFormat;
  }

  return ret;
}

/**
  * @brief  Set the LCD Active Layer.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  LCD layer index
  * @retval BSP status
  */
int32_t BSP_LCD_SetActiveLayer(uint32_t LayerIndex)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    Lcd_Ctx[Instance].ActiveLayer = LayerIndex;
  }

  return ret;
}

/**
  * @brief  Control the LTDC reload
  * @param  Instance    LCD Instance
  * @param  ReloadType can be one of the following values
  *         - BSP_LCD_RELOAD_NONE
  *         - BSP_LCD_RELOAD_IMMEDIATE
  *         - BSP_LCD_RELOAD_VERTICAL_BLANKING
  * @retval BSP status
  */
int32_t BSP_LCD_Relaod(uint32_t ReloadType)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else if(ReloadType == BSP_LCD_RELOAD_NONE)
  {
    Lcd_Ctx[Instance].ReloadEnable = 0U;
  }
  else if(HAL_LTDC_Reload (&hlcd_ltdc, ReloadType) != HAL_OK)
  {
    ret = BSP_ERROR_PERIPH_FAILURE;
  }
  else
  {
    Lcd_Ctx[Instance].ReloadEnable = 1U;
  }

  return ret;
}

/**
  * @brief  Sets an LCD Layer visible
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Visible Layer
  * @param  State  New state of the specified layer
  *          This parameter can be one of the following values:
  *            @arg  ENABLE
  *            @arg  DISABLE
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerVisible(uint32_t LayerIndex, FunctionalState State)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(State == ENABLE)
    {
      __HAL_LTDC_LAYER_ENABLE(&hlcd_ltdc, LayerIndex);
    }
    else
    {
      __HAL_LTDC_LAYER_DISABLE(&hlcd_ltdc, LayerIndex);
    }

    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      __HAL_LTDC_RELOAD_IMMEDIATE_CONFIG(&hlcd_ltdc);
    }
  }

  return ret;
}

/**
  * @brief  Configures the transparency.
  * @param  Instance      LCD Instance
  * @param  LayerIndex    Layer foreground or background.
  * @param  Transparency  Transparency
  *           This parameter must be a number between Min_Data = 0x00 and Max_Data = 0xFF
  * @retval BSP status
  */
int32_t BSP_LCD_SetTransparency(uint32_t LayerIndex, uint8_t Transparency)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      (void)HAL_LTDC_SetAlpha(&hlcd_ltdc, Transparency, LayerIndex);
    }
    else
    {
      (void)HAL_LTDC_SetAlpha_NoReload(&hlcd_ltdc, Transparency, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Sets an LCD layer frame buffer address.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer foreground or background
  * @param  Address     New LCD frame buffer value
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerAddress(uint32_t LayerIndex, uint32_t Address)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      (void)HAL_LTDC_SetAddress(&hlcd_ltdc, Address, LayerIndex);
    }
    else
    {
      (void)HAL_LTDC_SetAddress_NoReload(&hlcd_ltdc, Address, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Sets display window.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer index
  * @param  Xpos   LCD X position
  * @param  Ypos   LCD Y position
  * @param  Width  LCD window width
  * @param  Height LCD window height
  * @retval BSP status
  */
int32_t BSP_LCD_SetLayerWindow(uint16_t LayerIndex, uint16_t Xpos, uint16_t Ypos, uint16_t Width, uint16_t Height)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Reconfigure the layer size  and position */
      (void)HAL_LTDC_SetWindowSize(&hlcd_ltdc, Width, Height, LayerIndex);
      (void)HAL_LTDC_SetWindowPosition(&hlcd_ltdc, Xpos, Ypos, LayerIndex);
    }
    else
    {
      /* Reconfigure the layer size and position */
      (void)HAL_LTDC_SetWindowSize_NoReload(&hlcd_ltdc, Width, Height, LayerIndex);
      (void)HAL_LTDC_SetWindowPosition_NoReload(&hlcd_ltdc, Xpos, Ypos, LayerIndex);
    }

    Lcd_Ctx[Instance].XSize = Width;
    Lcd_Ctx[Instance].YSize = Height;
  }

  return ret;
}

/**
  * @brief  Configures and sets the color keying.
  * @param  Instance    LCD Instance
  * @param  LayerIndex  Layer foreground or background
  * @param  Color       Color reference
  * @retval BSP status
  */
int32_t BSP_LCD_SetColorKeying(uint32_t LayerIndex, uint32_t Color)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Configure and Enable the color Keying for LCD Layer */
      (void)HAL_LTDC_ConfigColorKeying(&hlcd_ltdc, Color, LayerIndex);
      (void)HAL_LTDC_EnableColorKeying(&hlcd_ltdc, LayerIndex);
    }
    else
    {
      /* Configure and Enable the color Keying for LCD Layer */
      (void)HAL_LTDC_ConfigColorKeying_NoReload(&hlcd_ltdc, Color, LayerIndex);
      (void)HAL_LTDC_EnableColorKeying_NoReload(&hlcd_ltdc, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Disables the color keying.
  * @param  Instance    LCD Instance
  * @param  LayerIndex Layer foreground or background
  * @retval BSP status
  */
int32_t BSP_LCD_ResetColorKeying(uint32_t LayerIndex)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    if(Lcd_Ctx[Instance].ReloadEnable == 1U)
    {
      /* Disable the color Keying for LCD Layer */
      (void)HAL_LTDC_DisableColorKeying(&hlcd_ltdc, LayerIndex);
    }
    else
    {
      /* Disable the color Keying for LCD Layer */
      (void)HAL_LTDC_DisableColorKeying_NoReload(&hlcd_ltdc, LayerIndex);
    }
  }

  return ret;
}

/**
  * @brief  Gets the LCD X size.
  * @param  Instance  LCD Instance
  * @param  XSize     LCD width
  * @retval BSP status
  */
int32_t BSP_LCD_GetXSize(uint32_t *XSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *XSize = Lcd_Ctx[Instance].XSize;
  }

  return ret;
}

/**
  * @brief  Gets the LCD Y size.
  * @param  Instance  LCD Instance
  * @param  YSize     LCD Height
  * @retval BSP status
  */
int32_t BSP_LCD_GetYSize(uint32_t *YSize)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *YSize = Lcd_Ctx[Instance].YSize;
  }

  return ret;
}

/**
  * @brief  Switch On the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOn(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    __HAL_LTDC_ENABLE(&hlcd_ltdc);

    /* Assert LCD_DISP_EN pin */
    HAL_GPIO_WritePin(LCD_DISP_EN_GPIO_PORT, LCD_DISP_EN_PIN, GPIO_PIN_SET);
    /* Assert LCD_BL_CTRL pin */
    HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
  }

  return ret;
}

/**
  * @brief  Switch Off the display.
  * @param  Instance    LCD Instance
  * @retval BSP status
  */
int32_t BSP_LCD_DisplayOff(uint32_t Instance)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    __HAL_LTDC_DISABLE(&hlcd_ltdc);
    /* Assert LCD_DISP_EN pin */
    HAL_GPIO_WritePin(LCD_DISP_EN_GPIO_PORT, LCD_DISP_EN_PIN, GPIO_PIN_RESET);
    /* Assert LCD_BL_CTRL pin */
    HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_RESET);
  }

  return ret;
}

/**
  * @brief  Set the brightness value
  * @param  Instance    LCD Instance
  * @param  Brightness [00: Min (black), 100 Max]
  * @retval BSP status
  */
int32_t BSP_LCD_SetBrightness(uint32_t Brightness)
{
  int32_t ret = BSP_ERROR_NONE;
    /* Timer Configuration */
    TIM_OC_InitTypeDef LCD_TIM_Config;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    /* Stop PWM Timer channel */
    HAL_TIM_PWM_Stop(&hlcd_tim, LCD_TIMx_CHANNEL);

    /* Common configuration for all channels */
    LCD_TIM_Config.OCMode       = TIM_OCMODE_PWM1;
    LCD_TIM_Config.OCPolarity   = TIM_OCPOLARITY_HIGH;
    LCD_TIM_Config.OCFastMode   = TIM_OCFAST_DISABLE;
    LCD_TIM_Config.OCNPolarity  = TIM_OCNPOLARITY_HIGH;
    LCD_TIM_Config.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    LCD_TIM_Config.OCIdleState  = TIM_OCIDLESTATE_RESET;

    /* Set the pulse value for channel */
    LCD_TIM_Config.Pulse =  (uint32_t)((LCD_TIMX_PERIOD_VALUE * Brightness) / 100);

    HAL_TIM_PWM_ConfigChannel(&hlcd_tim, &LCD_TIM_Config, LCD_TIMx_CHANNEL);

    /* Start PWM Timer channel */
    HAL_TIM_PWM_Start(&hlcd_tim, LCD_TIMx_CHANNEL);

    Lcd_Ctx[Instance].Brightness = Brightness;
  }

  return ret;
}

/**
  * @brief  Set the brightness value
  * @param  Instance    LCD Instance
  * @param  Brightness [00: Min (black), 100 Max]
  * @retval BSP status
  */
int32_t BSP_LCD_GetBrightness(uint32_t *Brightness)
{
  int32_t ret = BSP_ERROR_NONE;

  if(Instance >= LCD_INSTANCES_NBR)
  {
    ret = BSP_ERROR_WRONG_PARAM;
  }
  else
  {
    *Brightness = Lcd_Ctx[Instance].Brightness;
  }

  return ret;
}

/**
  * @brief  Draws a bitmap picture loaded in the internal Flash in currently active layer.
  * @param  Instance LCD Instance
  * @param  Xpos Bmp X position in the LCD
  * @param  Ypos Bmp Y position in the LCD
  * @param  pBmp Pointer to Bmp picture address in the internal Flash.
  * @retval BSP status
  */
int32_t BSP_LCD_DrawBitmap(uint32_t Xpos, uint32_t Ypos, uint8_t *pBmp)
{
  int32_t ret = BSP_ERROR_NONE;
  uint32_t index, width, height, bit_pixel;
  uint32_t Address;
  uint32_t input_color_mode;
  uint8_t *pbmp;

  /* Get bitmap data address offset */
  index = (uint32_t)pBmp[10] + ((uint32_t)pBmp[11] << 8) + ((uint32_t)pBmp[12] << 16)  + ((uint32_t)pBmp[13] << 24);

  /* Read bitmap width */
  width = (uint32_t)pBmp[18] + ((uint32_t)pBmp[19] << 8) + ((uint32_t)pBmp[20] << 16)  + ((uint32_t)pBmp[21] << 24);

  /* Read bitmap height */
  height = (uint32_t)pBmp[22] + ((uint32_t)pBmp[23] << 8) + ((uint32_t)pBmp[24] << 16)  + ((uint32_t)pBmp[25] << 24);

  /* Read bit/pixel */
  bit_pixel = (uint32_t)pBmp[28] + ((uint32_t)pBmp[29] << 8);

  /* Set the address */
  Address = hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (((Lcd_Ctx[Instance].XSize*Ypos) + Xpos)*Lcd_Ctx[Instance].BppFactor);

  /* Get the layer pixel format */
  if ((bit_pixel/8U) == 4U)
  {
    input_color_mode = DMA2D_INPUT_ARGB8888;
  }
  else if ((bit_pixel/8U) == 2U)
  {
    input_color_mode = DMA2D_INPUT_RGB565;
  }
  else
  {
    input_color_mode = DMA2D_INPUT_RGB888;
  }

  /* Bypass the bitmap header */
  pbmp = pBmp + (index + (width * (height - 1U) * (bit_pixel/8U)));

  /* Convert picture to ARGB8888 pixel format */
  for(index=0; index < height; index++)
  {
    /* Pixel format conversion */
    LL_ConvertLineToRGB((uint32_t *)pbmp, (uint32_t *)Address, width, input_color_mode);

    /* Increment the source and destination buffers */
    Address+=  (Lcd_Ctx[Instance].XSize * Lcd_Ctx[Instance].BppFactor);
    pbmp -= width*(bit_pixel/8U);
  }

  return ret;
}

/**
  * @brief  Draw a horizontal line on LCD.
  * @param  Instance LCD Instance.
  * @param  Xpos X position.
  * @param  Ypos Y position.
  * @param  pData Pointer to RGB line data
  * @param  Width Rectangle width.
  * @param  Height Rectangle Height.
  * @retval BSP status.
  */
int32_t BSP_LCD_FillRGBRect(uint32_t Xpos, uint32_t Ypos, uint8_t *pData, uint32_t Width, uint32_t Height)
{
  uint32_t i;
  uint8_t *pdata = pData;

#if (USE_DMA2D_TO_FILL_RGB_RECT == 1)
  uint32_t  Xaddress;
  for(i = 0; i < Height; i++)
  {
    /* Get the line address */
    Xaddress = hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (Lcd_Ctx[Instance].BppFactor*(((Lcd_Ctx[Instance].XSize + i)*Ypos) + Xpos));

    /* Write line */
    if(Lcd_Ctx[Instance].PixelFormat == LCD_PIXEL_FORMAT_RGB565)
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pdata, (uint32_t *)Xaddress, Width, DMA2D_INPUT_RGB565);
    }
    else
    {
      LL_ConvertLineToRGB(Instance, (uint32_t *)pdata, (uint32_t *)Xaddress, Width, DMA2D_INPUT_ARGB8888);
    }
    pdata += Lcd_Ctx[Instance].BppFactor*Width;
  }
#else
  uint32_t color, j;
  for(i = 0; i < Height; i++)
  {
    for(j = 0; j < Width; j++)
    {
      color = (uint32_t)((uint32_t)*pdata | ((uint32_t)(*(pdata + 1U)) << 8U) | ((uint32_t)(*(pdata + 2U)) << 16U) | ((uint32_t)(*(pdata + 3U)) << 24U));
      (void)BSP_LCD_WritePixel(Xpos + j, Ypos + i, color);
      pdata += Lcd_Ctx[Instance].BppFactor;
    }
  }
#endif

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws an horizontal line in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos  X position
  * @param  Ypos  Y position
  * @param  Length  Line length
  * @param  Color RGB color
  * @retval BSP status
  */
int32_t BSP_LCD_DrawHLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the line address */
  Xaddress = hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*Ypos) + Xpos));

  /* Write line */
  if((Xpos + Length) > Lcd_Ctx[Instance].XSize)
  {
    Length = Lcd_Ctx[Instance].XSize - Xpos;
  }
  LL_FillBuffer((uint32_t *)Xaddress, Length, 1, 0, Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a vertical line in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos  X position
  * @param  Ypos  Y position
  * @param  Length  Line length
  * @param  Color RGB color
  * @retval BSP status
  */
int32_t BSP_LCD_DrawVLine(uint32_t Xpos, uint32_t Ypos, uint32_t Length, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the line address */
  Xaddress = (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress) + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*Ypos) + Xpos));

  /* Write line */
  if((Ypos + Length) > Lcd_Ctx[Instance].YSize)
  {
    Length = Lcd_Ctx[Instance].YSize - Ypos;
  }
  LL_FillBuffer((uint32_t *)Xaddress, 1, Length, (Lcd_Ctx[Instance].XSize - 1U), Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a full rectangle in currently active layer.
  * @param  Instance   LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Width Rectangle width
  * @param  Height Rectangle height
  * @param  Color RGB color
  * @retval BSP status
  */
int32_t BSP_LCD_FillRect(uint32_t Xpos, uint32_t Ypos, uint32_t Width, uint32_t Height, uint32_t Color)
{
  uint32_t  Xaddress;

  /* Get the rectangle start address */
  Xaddress = (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress) + (Lcd_Ctx[Instance].BppFactor*((Lcd_Ctx[Instance].XSize*Ypos) + Xpos));

  /* Fill the rectangle */
  LL_FillBuffer((uint32_t *)Xaddress, Width, Height, (Lcd_Ctx[Instance].XSize - Width), Color);

  return BSP_ERROR_NONE;
}

/**
  * @brief  Reads a LCD pixel.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color RGB pixel color
  * @retval BSP status
  */
int32_t BSP_LCD_ReadPixel(uint32_t Xpos, uint32_t Ypos, uint32_t *Color)
{
  if(hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888)
  {
    /* Read data value from SDRAM memory */
    *Color = *(__IO uint32_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (4U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos)));
  }
  else /* if((hlcd_ltdc.LayerCfg[layer].PixelFormat == LTDC_PIXEL_FORMAT_RGB565) */
  {
    /* Read data value from SDRAM memory */
    *Color = *(__IO uint16_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (2U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos)));
  }

  return BSP_ERROR_NONE;
}

/**
  * @brief  Draws a pixel on LCD.
  * @param  Instance    LCD Instance
  * @param  Xpos X position
  * @param  Ypos Y position
  * @param  Color Pixel color
  * @retval BSP status
  */
int32_t BSP_LCD_WritePixel(uint32_t Xpos, uint32_t Ypos, uint32_t Color)
{
  if(hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].PixelFormat == LTDC_PIXEL_FORMAT_ARGB8888)
  {
    /* Write data value to SDRAM memory */
    *(__IO uint32_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (4U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos))) = Color;
  }
  else
  {
    /* Write data value to SDRAM memory */
    *(__IO uint16_t*) (hlcd_ltdc.LayerCfg[Lcd_Ctx[Instance].ActiveLayer].FBStartAdress + (2U*((Ypos*Lcd_Ctx[Instance].XSize) + Xpos))) = (uint16_t)Color;
  }

  return BSP_ERROR_NONE;
}

/*********************************************************************************************************
 LCD Private Functions
*********************************************************************************************************/

/**
  * @brief  Fills a buffer.
  * @param  Instance LCD Instance
  * @param  pDst Pointer to destination buffer
  * @param  xSize Buffer width
  * @param  ySize Buffer height
  * @param  OffLine Offset
  * @param  Color Color index
  */
static void LL_FillBuffer(uint32_t *pDst, uint32_t xSize, uint32_t ySize, uint32_t OffLine, uint32_t Color)
{
  uint32_t output_color_mode, input_color = Color;

  switch(Lcd_Ctx[Instance].PixelFormat)
  {
  case LCD_PIXEL_FORMAT_RGB565:
    output_color_mode = DMA2D_OUTPUT_RGB565; /* RGB565 */
    input_color = CONVERTRGB5652ARGB8888(Color);
    break;
  case LCD_PIXEL_FORMAT_RGB888:
  default:
    output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 */
    break;
  }

  /* Register to memory mode with ARGB8888 as color Mode */
  hlcd_dma2d.Init.Mode         = DMA2D_R2M;
  hlcd_dma2d.Init.ColorMode    = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = OffLine;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK)
  {
      if (HAL_DMA2D_Start(&hlcd_dma2d, input_color, (uint32_t)pDst, xSize, ySize) == HAL_OK)
      {
        /* Polling For DMA transfer */
      (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 50);
    }
  }
}

/**
  * @brief  Converts a line to an RGB pixel format.
  * @param  Instance LCD Instance
  * @param  pSrc Pointer to source buffer
  * @param  pDst Output color
  * @param  xSize Buffer width
  * @param  ColorMode Input color mode
  */
static void LL_ConvertLineToRGB(uint32_t *pSrc, uint32_t *pDst, uint32_t xSize, uint32_t ColorMode)
{
  uint32_t output_color_mode;

  switch(Lcd_Ctx[Instance].PixelFormat)
  {
  case LCD_PIXEL_FORMAT_RGB565:
    output_color_mode = DMA2D_OUTPUT_RGB565; /* RGB565 */
    break;
  case LCD_PIXEL_FORMAT_RGB888:
  default:
    output_color_mode = DMA2D_OUTPUT_ARGB8888; /* ARGB8888 */
    break;
  }

  /* Configure the DMA2D Mode, Color Mode and output offset */
  hlcd_dma2d.Init.Mode         = DMA2D_M2M_PFC;
  hlcd_dma2d.Init.ColorMode    = output_color_mode;
  hlcd_dma2d.Init.OutputOffset = 0;

  /* Foreground Configuration */
  hlcd_dma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hlcd_dma2d.LayerCfg[1].InputAlpha = 0xFF;
  hlcd_dma2d.LayerCfg[1].InputColorMode = ColorMode;
  hlcd_dma2d.LayerCfg[1].InputOffset = 0;

  hlcd_dma2d.Instance = DMA2D;

  /* DMA2D Initialization */
  if(HAL_DMA2D_Init(&hlcd_dma2d) == HAL_OK)
  {
    if(HAL_DMA2D_ConfigLayer(&hlcd_dma2d, 1) == HAL_OK)
    {
      if (HAL_DMA2D_Start(&hlcd_dma2d, (uint32_t)pSrc, (uint32_t)pDst, xSize, 1) == HAL_OK)
      {
        /* Polling For DMA transfer */
        (void)HAL_DMA2D_PollForTransfer(&hlcd_dma2d, 50);
      }
    }
  }
}

/**
  * @brief  Configure LCD control pins (Back-light, Display Enable and TE)
  * @retval None
  */
static void LCD_InitSequence(void)
{
  GPIO_InitTypeDef  gpio_init_structure;

  /* LCD_DISP GPIO configuration */
  LCD_DISP_EN_GPIO_CLK_ENABLE();
  gpio_init_structure.Pin       = LCD_DISP_EN_PIN;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_DISP_EN_GPIO_PORT, &gpio_init_structure);
  /* Assert LCD_DISP  pin */
  HAL_GPIO_WritePin(LCD_DISP_EN_GPIO_PORT, LCD_DISP_EN_PIN, GPIO_PIN_SET);

  /* LCD_BL_CTRL GPIO configuration */
  LCD_BL_CTRL_GPIO_CLK_ENABLE();
  gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;
  gpio_init_structure.Mode      = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &gpio_init_structure);
  /* Assert back-light LCD_BL_CTRL pin */
  HAL_GPIO_WritePin(LCD_BL_CTRL_GPIO_PORT, LCD_BL_CTRL_PIN, GPIO_PIN_SET);
}

/**
  * @brief  DeInitializes LCD GPIO special pins MSP.
  * @retval None
  */
static void LCD_DeInitSequence(void)
{
  GPIO_InitTypeDef  gpio_init_structure;

  /* LCD_DISP GPIO configuration */
  /* LCD_RESET GPIO deactivation */
  gpio_init_structure.Pin       = LCD_DISP_EN_PIN;
  HAL_GPIO_DeInit(LCD_DISP_EN_GPIO_PORT, gpio_init_structure.Pin);

  /* LCD_BL_CTRL GPIO configuration */
  /* LCD_BL_CTRL GPIO deactivation */
  gpio_init_structure.Pin       = LCD_BL_CTRL_PIN;
  HAL_GPIO_DeInit(LCD_BL_CTRL_GPIO_PORT, gpio_init_structure.Pin);
}

/*********************************************************************************************************
 BSP Routines: LTDC & DMA2D
*********************************************************************************************************/

/**
  * @brief  Initialize the BSP LTDC Msp.
  * @param  hltdc  LTDC handle
  * @retval None
  */
static void LTDC_MspInit(LTDC_HandleTypeDef *hltdc)
{
    GPIO_InitTypeDef  gpio_init_structure;

    if(hltdc->Instance == LTDC)
    {
      /** Enable the LTDC clock */
      __HAL_RCC_LTDC_CLK_ENABLE();

      /* -----------------------------
       * PA3     LTDC_B5         AF14
       * PA4     LTDC_VSYNC      AF14
       * PA6     LTDC_G2         AF14
       * PA10    LTDC_B4         AF12
       * PA11    LTDC_R4         AF14
       *
       * PB0     LTDC_R3         AF9
       * PB1     LTDC_R6         AF9
       * PB8     LTDC_B6         AF14
       * PB9     LTDC_B7         AF14
       * PB10    LTDC_G4         AF14
       * PB11    LTDC_G5         AF14
       *
       * PC0     LTDC_R5         AF14
       * PC6     LTDC_HSYNC      AF14
       * PC7     LTDC_G6         AF14
       *
       * PD3     LTDC_G7         AF14
       * PD10    LTDC_B3         AF14
       *
       * PE11    LTDC_G3         AF14
       * PE13    LTDC_DISPEN     AF14
       * PE14    LTDC_CLK        AF14
       * PE15    LTDC_R7         AF14
       * -------------------------- */

      /* Enable GPIOs clock */
      __HAL_RCC_GPIOA_CLK_ENABLE();
      __HAL_RCC_GPIOB_CLK_ENABLE();
      __HAL_RCC_GPIOC_CLK_ENABLE();
      __HAL_RCC_GPIOD_CLK_ENABLE();
      __HAL_RCC_GPIOE_CLK_ENABLE();

      /*** LTDC Pins configuration ***/
      /* GPIOA configuration */
      gpio_init_structure.Pin       = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_11;
      gpio_init_structure.Mode      = GPIO_MODE_AF_PP;
      gpio_init_structure.Pull      = GPIO_NOPULL;
      gpio_init_structure.Speed     = GPIO_SPEED_FREQ_HIGH;
      gpio_init_structure.Alternate = GPIO_AF14_LTDC;
      HAL_GPIO_Init(GPIOA, &gpio_init_structure);
      gpio_init_structure.Pin       = GPIO_PIN_10;
      gpio_init_structure.Alternate = GPIO_AF12_LTDC;
      HAL_GPIO_Init(GPIOA, &gpio_init_structure);

      /* GPIOB configuration */
      gpio_init_structure.Pin      = GPIO_PIN_0 | GPIO_PIN_1;
      gpio_init_structure.Alternate = GPIO_AF9_LTDC;
      HAL_GPIO_Init(GPIOB, &gpio_init_structure);
      gpio_init_structure.Pin      = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
      gpio_init_structure.Alternate = GPIO_AF14_LTDC;
      HAL_GPIO_Init(GPIOB, &gpio_init_structure);

      /* GPIOC configuration */
      gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_6 | GPIO_PIN_7;
      gpio_init_structure.Alternate = GPIO_AF14_LTDC;
      HAL_GPIO_Init(GPIOC, &gpio_init_structure);

      /* GPIOD configuration */
      gpio_init_structure.Pin       = GPIO_PIN_3 | GPIO_PIN_10;
      gpio_init_structure.Alternate = GPIO_AF14_LTDC;
      HAL_GPIO_Init(GPIOD, &gpio_init_structure);

      /* GPIOE configuration */
      gpio_init_structure.Pin       = GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
      gpio_init_structure.Alternate = GPIO_AF14_LTDC;
      HAL_GPIO_Init(GPIOE, &gpio_init_structure);

      /** Toggle Sw reset of LTDC IP */
      __HAL_RCC_LTDC_FORCE_RESET();
      __HAL_RCC_LTDC_RELEASE_RESET();
    }
}

/**
  * @brief  De-Initializes the BSP LTDC Msp
  * @param  hltdc  LTDC handle
  * @retval None
  */
static void LTDC_MspDeInit(LTDC_HandleTypeDef *hltdc)
{
    GPIO_InitTypeDef  gpio_init_structure;

    if(hltdc->Instance == LTDC)
    {
      /* LTDC Pins deactivation */
      /* GPIOA deactivation */
      gpio_init_structure.Pin       = GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_6 | GPIO_PIN_11 | GPIO_PIN_10;
      HAL_GPIO_DeInit(GPIOA, gpio_init_structure.Pin);

      /* GPIOB deactivation */
      gpio_init_structure.Pin      = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
      HAL_GPIO_DeInit(GPIOB, gpio_init_structure.Pin);

      /* GPIOC deactivation */
      gpio_init_structure.Pin       = GPIO_PIN_0 | GPIO_PIN_6 | GPIO_PIN_7;
      HAL_GPIO_DeInit(GPIOC, gpio_init_structure.Pin);

      /* GPIOD deactivation */
      gpio_init_structure.Pin       = GPIO_PIN_3 | GPIO_PIN_10;
      HAL_GPIO_DeInit(GPIOD, gpio_init_structure.Pin);

      /* GPIOE deactivation */
      gpio_init_structure.Pin       = GPIO_PIN_11 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
      HAL_GPIO_DeInit(GPIOE, gpio_init_structure.Pin);

      /** Force and let in reset state LTDC */
      __HAL_RCC_LTDC_FORCE_RESET();

      /** Disable the LTDC */
      __HAL_RCC_LTDC_CLK_DISABLE();
    }
}

/**
  * @brief  Initialize the BSP DMA2D Msp.
  * @param  hdma2d  DMA2D handle
  * @retval None
  */
static void DMA2D_MspInit(DMA2D_HandleTypeDef *hdma2d)
{
  if(hdma2d->Instance == DMA2D)
  {
    /** Enable the DMA2D clock */
    __HAL_RCC_DMA2D_CLK_ENABLE();

    /** Toggle Sw reset of DMA2D IP */
    __HAL_RCC_DMA2D_FORCE_RESET();
    __HAL_RCC_DMA2D_RELEASE_RESET();
  }
}

/**
  * @brief  De-Initializes the BSP DMA2D Msp
  * @param  hdma2d  DMA2D handle
  * @retval None
  */
static void DMA2D_MspDeInit(DMA2D_HandleTypeDef *hdma2d)
{
  if(hdma2d->Instance == DMA2D)
  {
    /** Disable IRQ of DMA2D IP */
    HAL_NVIC_DisableIRQ(DMA2D_IRQn);

    /** Force and let in reset state DMA2D */
    __HAL_RCC_DMA2D_FORCE_RESET();

    /** Disable the DMA2D */
    __HAL_RCC_DMA2D_CLK_DISABLE();
  }
}

/**
  * @brief  Initializes TIM MSP.
  * @param  htim  TIM handle
  * @retval None
  */
static void TIMx_PWM_MspInit(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  GPIO_InitTypeDef GPIO_InitStruct;

  LCD_BL_CTRL_GPIO_CLK_ENABLE();

  /* TIMx Peripheral clock enable */
  LCD_TIMx_CLK_ENABLE();

  /* Timer channel configuration */
  GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull      = GPIO_NOPULL;
  GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_MEDIUM;
  GPIO_InitStruct.Alternate = LCD_TIMx_CHANNEL_AF;
  GPIO_InitStruct.Pin       = LCD_BL_CTRL_PIN; /* BL_CTRL */

  HAL_GPIO_Init(LCD_BL_CTRL_GPIO_PORT, &GPIO_InitStruct);
}

/**
  * @brief  De-Initializes TIM MSP.
  * @param  htim TIM handle
  * @retval None
  */
static void TIMx_PWM_MspDeInit(TIM_HandleTypeDef *htim)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(htim);

  GPIO_InitTypeDef GPIO_InitStruct;

  /* TIMx Peripheral clock enable */
  LCD_BL_CTRL_GPIO_CLK_DISABLE();

  /* Timer channel configuration */
  GPIO_InitStruct.Pin = LCD_BL_CTRL_PIN; /* BL_CTRL */
  HAL_GPIO_DeInit(LCD_BL_CTRL_GPIO_PORT, GPIO_InitStruct.Pin);
}

/**
  * @brief  Initializes TIM in PWM mode
  * @param  htim TIM handle
  * @retval None
  */
static void TIMx_PWM_Init(TIM_HandleTypeDef *htim)
{
  /* Timer_Clock = 2 x  APB2_clock = 200 MHz */
  /* PWM_freq = Timer_Clock /(Period x (Prescaler + 1))*/
  /* PWM_freq = 200 MHz /(50000 x (4 + 1)) = 800 Hz*/
  htim->Instance = LCD_TIMx;
  (void)HAL_TIM_PWM_DeInit(htim);

  TIMx_PWM_MspInit(htim);

  htim->Init.Prescaler         = LCD_TIMX_PRESCALER_VALUE;
  htim->Init.Period            = LCD_TIMX_PERIOD_VALUE;
  htim->Init.ClockDivision     = 0;
  htim->Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim->Init.RepetitionCounter = 0;
  htim->Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  (void)HAL_TIM_PWM_Init(htim);
}

/**
  * @brief  De-Initializes TIM in PWM mode
  * @param  htim TIM handle
  * @retval None
  */
static void TIMx_PWM_DeInit(TIM_HandleTypeDef *htim)
{
  htim->Instance = LCD_TIMx;

  /* Timer de-intialization */
  (void)HAL_TIM_PWM_DeInit(htim);

  /* Timer Msp de-intialization */
  TIMx_PWM_MspDeInit(htim);
}

/*********************************************************************************************************
   END
*********************************************************************************************************/
