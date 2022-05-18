/**
 * @file
 *
 * @brief Support file for CUnit where the suite registration functions are declared.
 *
 * @see test.c
 */

#ifndef TEST_REGISTRATION
#define TEST_REGISTRATION

/**
 * Register the timer suite
 */
void register_timer(void);

/**
 * Register the build suite
 */
void register_build(void);

/**
 * Register the entropy codec (coder and decoder) suite
 */
void register_entropy_codec(void);

/**
 * Register the file suite
 */
void register_file(void);

/**
 * Register the quantization suite
 */
void register_quantizer(void);

/**
 * Register the decorrelation suite
 */
void register_decorrelator(void);

/**
 * Register the compressor_decompressor suite
 */
void register_compressor_decompressor(void);

/**
 * Register the bin_common suite
 */
void register_bin_common(void);


#endif

