/*
 * E Ink - pixel process library
 *
 * Copyright (C) 2020 E Ink Holdings Inc.
 *
 */

typedef enum _EInkRenderErrMsg {
	EINK_SUCESS = 0,
	EINK_PANEL_SIZE_WRONG,
	EINK_FILE_SIZE_WRONG,
	EINK_FILE_CORRUPTION,
	EINK_COLOR_TYPE_WRONG,
	EINK_NOT_INITIAL,
}EInkRenderErrMsg;


typedef enum _ImageFormat {
	RGBA8888,
    ARGB8888
}ImageFormat;



/**
 * @brief Library Initial
 * 
 * @param[in] fileBuffer read CFA_LUT binary file to byte buffer
 * @param[in] bufferSize length of fileBuffer
 * @param[in] PANEL_WIDTH Panel width
 * @param[in] PANEL_HEIGHT Panel heihgt
 * 
 * @return EInk Error Massage
 */
EInkRenderErrMsg EInk_Init(uint8_t* fileBuffer, uint32_t bufferSize, int PANEL_WIDTH, int PANEL_HEIGHT);


/**
 * @brief color mapping with Gain
 * 
 * @note Must run EInk_Init() before
 * 
 * @param[in] color_buffer Input image.
 * @param[in] ImageFormat The Format of color_buffer
 * @param[out] gray_buffer Output Y8(256) image.
 * @param[in] width The width of both images.
 * @param[in] height The height of both images.
 * @param[in] gain enhance number, 1.0 ~ 2.0.
 * @param[in] left left of region rectangle.
 * @param[in] top top of region rectangle.
 * @param[in] right right of region rectangle.
 * @param[in] bottom bottom of region rectangle.
 * @param[in] rotate 
 * 				0 - 0 degree, 
 * 				1 - 90 degree, 
 * 				2 - 180 degree, 
 * 				3 - 270 degree, 
 * 				4 - upside down, 
 * 				5 - reverse right and left
 * 
 * @return EInk Error Massage
 */
EInkRenderErrMsg eink_color_mapping_Gain(unsigned char *color_buffer, ImageFormat inputType, unsigned char *gray_buffer, uint32_t width, uint32_t height, unsigned int gain, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, int rotate);


/**
 * @brief color mapping with AIE
 * 
 * @note Must run EInk_Init() before
 * 
 * @param[in] color_buffer Input image.
 * @param[in] ImageFormat The Format of color_buffer.
 * @param[out] gray_buffer Output Y8(256) image.
 * @param[in] width The width of both images.
 * @param[in] height The height of both images.
 * @param[in] lut AIE Lookup table provided by EInk.
 * @param[in] left left of region rectangle.
 * @param[in] top top of region rectangle.
 * @param[in] right right of region rectangle. 
 * @param[in] bottom bottom of region rectangle. 
 * @param[in] rotate 
 * 				0 - 0 degree, 
 * 				1 - 90 degree, 
 * 				2 - 180 degree, 
 * 				3 - 270 degree, 
 * 				4 - upside down, 
 * 				5 - reverse right and left
 * 
 * @return EInk Error Massage
 */
EInkRenderErrMsg eink_color_mapping_AIE(unsigned char *color_buffer, ImageFormat inputType, unsigned char *gray_buffer, uint32_t width, uint32_t height, unsigned char *lut, uint32_t left, uint32_t top, uint32_t right, uint32_t bottom, int rotate);

/**
 * @brief Get the build date of this library.
 *   
 * @return The build date of the library as a 32-bit integer.
 */
uint32_t Eink_get_DATE(void);
