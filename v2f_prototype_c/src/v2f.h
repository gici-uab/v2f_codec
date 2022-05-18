/**
 * @file
 *
 * @brief Public interface of the V2F compression library.
 *
 * This file provides the external interface from the core library in src/ to the external world. These functions
 * are expected to operate without crashing under _any_ user provided data. All inputs are validated and proper
 * failure codes are returned in case of error, with the exception of an invalid memory pointers, for which it is not
 * possible to tell whether they point to allocated memory or not.
 *
 * No memory allocation nor file operation is performed in the functions declared in this file.
 * See @a v2f_file.h for functions operating directly on files.
 */

#ifndef V2F_H
#define V2F_H

// TODO: prerelease: fix version string once released
/**
 * Software version number.
 */
#define PROJECT_VERSION "20210801"

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

// Exported constants
// Note: some constants employ #define because an equivalent enum is not guaranteed not to overflow.

/// @name Global definitions

/// Unsigned sampled value. Entropy coders/decoders use these to represent data.
typedef uint32_t v2f_sample_t;
/// Maximum value that can be stored in this type
#define V2F_SAMPLE_T_MAX UINT32_MAX

/**
 * Signed sample value. Decorrelation may produce these as intermediate values
 * before sign coding back to v2f_sample_t. Note that in the general pipeline
 * outside decorrelation, only v2f_sample_t sample values are and should be used.
 */
typedef int32_t v2f_signed_sample_t;

// Enums

/**
 * @enum v2f_error_t
 *
 * Functions in this software may return one of these error codes.
 *
 * This struct must be kept in sync with the @ref v2f_error_strings variable in errors.c
 */
typedef enum {
    V2F_E_NONE = 0,                                     ///< Function returned no error.
    V2F_E_UNEXPECTED_END_OF_FILE = 1,                   ///< End of file found unexpectedly.
    V2F_E_IO = 2,                                       ///< An error occurred while performing an I/O operation.
    V2F_E_CORRUPTED_DATA = 3,                           ///< File is corrupted or syntactically invalid.
    V2F_E_INVALID_PARAMETER = 4,                        ///< Input parameters are not valid.
    V2F_E_NON_ZERO_RESERVED_OR_PADDING = 5,             ///< Non-zero value found at reserved/padding positions.
    V2F_E_UNABLE_TO_CREATE_TEMPORARY_FILE = 6,          ///< An error occurred while creating a temporary file.
    V2F_E_OUT_OF_MEMORY = 7,                            ///< Program ran out of memory
    /// An application-specific implementation was requested, but is not implemented.
    V2F_E_FEATURE_NOT_IMPLEMENTED = 8,                  // This one needs to be always the last one.
} v2f_error_t;

/// Maximum number of entries in a V2F tree or forest
#define V2F_C_MAX_ENTRY_COUNT (UINT32_MAX - 1)

/**
 * @enum v2f_entropy_constants_t
 *
 * Constants related to the entropy coders/decoders,
 * common to the whole application.
 */
typedef enum {
    /// @name Input sample limits
    /// Minimum number of bytes allowed to represent each original sample
    V2F_C_MIN_BYTES_PER_SAMPLE = 1,
    /// Maximum number of bytes allowed to represent each original sample
    V2F_C_MAX_BYTES_PER_SAMPLE = 2,
    /// Maximum supported sample value
    V2F_C_MAX_SAMPLE_VALUE = ((v2f_sample_t) ((UINT32_C(1)
            << (8 * V2F_C_MAX_BYTES_PER_SAMPLE)) - 1)),
    /**
     * Minimum @ref v2f_signed_sample_t value that can be sign coded
     * and still fit in a v2f_sample_t.
     */
    V2F_C_MIN_SIGNED_VALUE = INT32_MIN + 1,
    /**
     * Maximum @ref v2f_signed_sample_t that can be sign coded
     * and still fit in a v2f_sample_t.
     */
    V2F_C_MAX_SIGNED_VALUE = INT32_MAX,

    /// @name Compressed codeword size limits
    /// Minimum number of bytes used to represent an output codeword
    V2F_C_MIN_BYTES_PER_WORD = 1,
    /// Maximum number of bytes used to represent an output codeword
    V2F_C_MAX_BYTES_PER_WORD = 2,

    /// @name Samples per codeword size limits
    // Minimum number of samples represented by an entry
    V2F_C_MIN_SAMPLE_COUNT = 1,
    // Maximum number of samples represented by an entry
    V2F_C_MAX_SAMPLE_COUNT = UINT16_MAX,

    /// @name V2F forest size bounds
    /// Minimum number of entries in a V2F tree or forest
    V2F_C_MIN_ENTRY_COUNT = 2,
    // Maximum number of entries in a V2F tree or forest
    // V2F_C_MAX_ENTRY_COUNT = (defined above because enums are restricted to int)
    /// Minimum number of root entries in a V2F codec
    V2F_C_MIN_ROOT_COUNT = 1,
    /// Maximum number of root entries in a V2F codec
    V2F_C_MAX_ROOT_COUNT = V2F_C_MAX_SAMPLE_VALUE + 1,
    // Maximum number of children in an entry
    V2F_C_MAX_CHILD_COUNT = V2F_C_MAX_SAMPLE_VALUE + 1,

    /// @name Input/output block size limits
    /// Minimum number of samples allowed in a block
    V2F_C_MIN_BLOCK_SIZE = 1,
    /// Maximum number of samples allowed in a block
    V2F_C_MAX_BLOCK_SIZE = 5120 * 256,

    /// Maximum number of bytes in a compressed block, i.e., one word per sample
    V2F_C_MAX_COMPRESSED_BLOCK_SIZE =
    V2F_C_MAX_BLOCK_SIZE * V2F_C_MAX_BYTES_PER_WORD,
} v2f_entropy_constants_t;

/// @name Quantizer-related definitions

/**
 * @enum v2f_quantizer_mode_t
 *
 * Types of quantization defined.
 */
typedef enum {
    /// Null quantizer, that does not modify the data
    V2F_C_QUANTIZER_MODE_NONE = 0,
    /// Uniform scalar quantizer
    V2F_C_QUANTIZER_MODE_UNIFORM = 1,
    /// Number of quantizer modes available
    V2F_C_QUANTIZER_MODE_COUNT = 2,
} v2f_quantizer_mode_t;

/**
 * @enum v2f_quantizer_constant_t
 *
 * Constants that bound the parameters of the quantizer.
 */
typedef enum {
    /// Maximum quantization step size allowed by this module.
    V2F_C_QUANTIZER_MODE_MAX_STEP_SIZE = 255
} v2f_quantizer_constant_t;


/**
 * @struct v2f_quantizer_t
 *
 * Represent a quantizer of input samples.
 */
typedef struct {
    /// Index to identify the quantization being applied
    v2f_quantizer_mode_t mode;
    /**
     * Maximum number of input sample values per quantization bin.
     * Set to 1 for no quantization.
     */
    v2f_sample_t step_size;
    /**
     * Maximum sample value allowed at the input.
     * It is used for reconstruction only, to avoid exceeding
     * the dynamic range for the last quantization bin.
     */
     v2f_sample_t max_sample_value;
} v2f_quantizer_t;

/// @name Decorrelation-related definitions

/**
 * @enum v2f_decorrelator_mode_t
 *
 * List of defined decorrelation modes.
 */
typedef enum {
    /// Identity decorrelator, that does not modify the input samples
    V2F_C_DECORRELATOR_MODE_NONE = 0,
    /// DPCM decorrelator of order one, using the sample to the left
    V2F_C_DECORRELATOR_MODE_LEFT = 1,
    /**
     * DPCM decorrelator of order two, using the average of
     * the two samples to the left
     */
    V2F_C_DECORRELATOR_MODE_2_LEFT = 2,
    /// Decorrelator using JPEG-LS
    V2F_C_DECORRELATOR_MODE_JPEG_LS = 3,
    /** Decorrelator using the average of two left, left, left-north
     * and north samples
     */
    V2F_C_DECORRELATOR_MODE_FGIJ = 4,

    /// Number of available decorrelation modes
    V2F_C_DECORRELATOR_MODE_COUNT = 5,
} v2f_decorrelator_mode_t;

/**
 * @struct v2f_decorrelator_t
 *
 * Represent a decorrelator, one of the stages of the compression pipeline.
 */
typedef struct {
    /// Decorrelation mode
    v2f_decorrelator_mode_t mode;

    /// Maximum original sample value
    v2f_sample_t max_sample_value;

    /**
     * Samples per row, aka stride. If 0 or equal to the number of samples,
     * the input will be processed as a single row matrix. This is the default.
     * Prediction modes that require 2D geometry information can use
     * this value to perform their computations.
     */
    uint64_t samples_per_row;
} v2f_decorrelator_t;

/// @name Entropy coding definitions

/**
 * @struct v2f_entropy_coder_entry_t
 *
 * Each of the individual table entries, corresponding to an included
 * element in a V2F tree.
 */
typedef struct v2f_entropy_coder_entry_t {
    /// List of `children_count` pointers to other v2f_entropy_coder_entry_t instances
    struct v2f_entropy_coder_entry_t **children_entries;
    /// Number of children instances
    uint32_t children_count;
    /**
     * Pointer to a buffer with the bytes corresponding to this entry's
     * word bytes. Emitting this entry consists in outputing these bytes.
     */
    uint8_t *word_bytes;
} v2f_entropy_coder_entry_t;

/**
 * @struct v2f_entropy_coder_t
 *
 * Represent a generic variable to fixed (V2F) coder
 */
typedef struct {
    /// @name General coder parameters

    /// Number of bytes used to represent the word of an included treee node.
    uint8_t bytes_per_word;
    /// Maximum sample value expected by this coder.
    v2f_sample_t max_expected_value;

    /// @name Forest structure

    /// List of root entries for this coder (don't count towards `entry_cont`).
    v2f_entropy_coder_entry_t **roots;
    /// Number of root entries in the coder.
    uint32_t root_count;

    /// @name Auxiliary for efficient coding

    /// Current node in the V2F forest
    v2f_entropy_coder_entry_t *current_entry;
} v2f_entropy_coder_t;

/// @name Entropy decoding definitions

/**
 * @struct v2f_entropy_decoder_entry_t
 *
 * Each of the entries within a V2F tree.
 */
typedef struct {
    /// Samples associated to this entry
    v2f_sample_t *samples;
    /// Number of samples associated to this entry
    uint32_t sample_count;
    /// Number of children of this entry
    uint32_t children_count;
    /**
     * Pointer to the twin entry in the corresponding coder.
     * (Not used during decompression, only when dumping the coder/decoder
     * pair with v2f_file.h
     */
    v2f_entropy_coder_entry_t *coder_entry;
} v2f_entropy_decoder_entry_t;

/**
 * @struct v2f_entropy_decoder_root_t
 *
 * This type gives access to all entries within a V2F tree.
 */
typedef struct {
    /// Array of entries ordered by index value
    v2f_entropy_decoder_entry_t *entries_by_index;
    /// Total number of entries in `entries_by_index`.
    uint32_t root_entry_count;

    /**
      * Array of pointers to included nodes,
      * so that they can be properly decoded.
      */
    v2f_entropy_decoder_entry_t **entries_by_word;
    /**
     * Total number of entries in `entries_by_word` which have
     * an associated codeword. These are indexed by the codewords
     * to be read from compressed blocks (unsigned, big endian,
     * `bytes_per_word` bytes).
     */
    uint32_t root_included_count;

} v2f_entropy_decoder_root_t;

/**
 * @struct v2f_entropy_decoder_t
 *
 * Represent a V2F decoder
 */
typedef struct {
    /// Number of bytes per index expected in the compressed data
    uint8_t bytes_per_word;
    /// Number of bytes used to represent each original sample value
    uint8_t bytes_per_sample;

    /**
     * List of root nodes, indexed by a simple index s.
     * - Root [s] should be able to code any symbol >= s.
     * - Roots can be aliased (multiple pointers to the same root)
     * - There should be at least m elements in this list if m-1 is
     *   the maximum expected sample value.
     */
    v2f_entropy_decoder_root_t **roots;
    /// Number of roots in this decoder
    uint32_t root_count;
    /// Auxiliary pointer to the current root, useful for efficient decoding.
    v2f_entropy_decoder_root_t *current_root;

    /// Auxiliary pointer to the the null entry, needed to avoid memory leaks.
    v2f_entropy_coder_entry_t **null_entry;
} v2f_entropy_decoder_t;

/// @name Compressor definitions

/**
 * @struct v2f_compressor_t
 *
 * Represent a complete compression pipeline.
 */
typedef struct v2f_compressor_t {
    /// Pointer to the quantizer to be used.
    v2f_quantizer_t *quantizer;
    /// Pointer to the decorrelator to be used.
    v2f_decorrelator_t *decorrelator;
    /// Pointer to the entropy coder to be used.
    v2f_entropy_coder_t *entropy_coder;
} v2f_compressor_t;

/// @name Decompressor definitions

/**
 * @struct v2f_decompressor_t
 *
 * Represent a complete compression pipeline.
 */
typedef struct v2f_decompressor_t {
    /// Pointer to the quantizer to be used
    v2f_quantizer_t *quantizer;
    /// Pointer to the decorrelator to be used
    v2f_decorrelator_t *decorrelator;
    /// Pointer to the entropy decoder to be used
    v2f_entropy_decoder_t *entropy_decoder;
} v2f_decompressor_t;

/// @name File-level operation definitions

/**
 * @enum v2f_dict_file_constant_t
 *
 * Constants related to files representing V2F forests
 */
typedef enum {
    /// Number of bytes per index. Fixed size in the header file for simplicity.
    V2F_C_BYTES_PER_INDEX = 4,
} v2f_dict_file_constant_t;

/// @name Error-related definitions

#include "errors.h"

/// @name Public functions exported in the generated libraries.

/**
 * The V2F_EXPORTED_SYMBOL macro limits to symbol visibility so that only relevant functions are exported.
 */
#ifdef __GNUC__
#define V2F_EXPORTED_SYMBOL __attribute__((__visibility__("default"))) __attribute__ ((warn_unused_result))
// Add support for other platforms here. E.g.,
//#elif _MSC_VER
//#define V2F_EXPORTED_SYMBOL __declspec(dllexport)
#else
#define V2F_EXPORTED_SYMBOL
#endif

// Functions

/**
 * Compress @a raw_file_path into @a utput_file_path using the V2F codec
 * configuration defined in @a output_file_path.
 * Part of this configuration can be overwriten with the provided parameters.
 *
 * The contents of @a header_file_path are not included in the
 * compressed file. To decompress it, a copy of @a header_file_path
 * must be available at the decoder side,
 * along with the knowledge of any overwriten parameters.
 *
 * Please refer to the user manual for additional information on file formats
 * and other details.
 *
 * @param raw_file_path path to the file with the raw data to compress.
 * @param header_file_path path to the (typically .v2fc) file with
 *   the codec definition.
 * @param output_file_path path where the compressed data are to be stored.
 *
 * @param overwrite_quantizer_mode if true, the quantizer mode defined in
 *   @a header_file_path is overwritten.
 * @param quantizer_mode if @a overwrite_quantizer_mode is true, this mode
 *   is employed for compression. Otherwise, it is ignored.
 * @param overwrite_qstep if true, the quantizer step size defined in
 *   @a header_file_path is overwritten.
 * @param step_size if @a overwrite_qstep is true, and if the effective
 *   quantization mode is not NULL quantization, this is the step size
 *   employed for quantization. Otherwise, it is ignored.
 * @param overwrite_decorrelator_mode if true, the decorrelator mode defined in
 *   @a header_file_path is overwritten.
 * @param decorrelator_mode if @a overwrite_decorrelator_mode is true,
 *   this is the decorrelation mode empoyed during compression. Otherwise,
 *   it is ignored.
 * @param samples_per_row number of samples per row
 * @param shadow_y_pairs if not NULL, it must be a non-empty array with an even number of integers
 *   describing horizontal shadow regions. The format of the array must be
 *   start1,end1,start2,end2,...,startN,endN
 *   where the first shadow region is at rows (y-positions) start1 to end1 (both included)
 *   and so on. Shadow regions must be provided from lower to higher values of y,
 *   and there can be no overlap between them.
 * @param y_shadow_count number of y shadow regions. If y_shadow_count > 1,
 *   then the shadow_y_pairs is expected not to be NULL and to contain exactly
 *   twice as many elements. If shadow_y_paris is NULL, then y_shadow_count must be 0.
 *
 * @return 0 if and only if compression was successful.
 */
V2F_EXPORTED_SYMBOL
int v2f_file_compress_from_path(
        char const *const raw_file_path,
        char const *const header_file_path,
        char const *const output_file_path,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode,
        v2f_sample_t samples_per_row,
        uint32_t* shadow_y_pairs,
        uint32_t y_shadow_count);

/**
 * Compresses an open file into another, using an open header file.
 * It is otherwise identical in behavior to @ref v2f_file_compress_from_path.
 *
 * Please refer to the user manual for additional information on file formats
 * and other details.
 * 
 * @param raw_file file open for reading with the data to be read.
 *   All remaining data are consumed.
 * @param header_file file open for reading with the V2F codec
 *   defintion
 * @param output_file file open for writing where the compressed
 *   data is to be stored.
 *
 * @param overwrite_quantizer_mode if true, the quantizer mode defined in
 *   @a header_file_path is overwritten.
 * @param quantizer_mode if @a overwrite_quantizer_mode is true, this mode
 *   is employed for compression. Otherwise, it is ignored.
 * @param overwrite_qstep if true, the quantizer step size defined in
 *   @a header_file_path is overwritten.
 * @param step_size if @a overwrite_qstep is true, and if the effective
 *   quantization mode is not NULL quantization, this is the step size
 *   employed for quantization. Otherwise, it is ignored.
 * @param overwrite_decorrelator_mode if true, the decorrelator mode defined in
 *   @a header_file_path is overwritten.
 * @param decorrelator_mode if @a overwrite_decorrelator_mode is true,
 *   this is the decorrelation mode empoyed during compression. Otherwise,
 *   it is ignored.
 * @param samples_per_row number of samples per row
 * @param shadow_y_pairs if not NULL, it must be a non-empty array with an even number of integers
 *   describing horizontal shadow regions. The format of the array must be
 *   start1,end1,start2,end2,...,startN,endN
 *   where the first shadow region is at rows (y-positions) start1 to end1 (both included)
 *   and so on.
 * @param y_shadow_count number of y shadow regions. If y_shadow_count > 1,
 *   then the shadow_y_pairs is expected not to be NULL and to contain exactly
 *   twice as many elements. If shadow_y_paris is NULL, then y_shadow_count must be 0.
 *
 * @return 0 if and only if compression was successful
 */
V2F_EXPORTED_SYMBOL
int v2f_file_compress_from_file(
        FILE *raw_file,
        FILE *header_file,
        FILE *output_file,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode,
        v2f_sample_t samples_per_row,
        uint32_t* shadow_y_pairs,
        uint32_t y_shadow_count);

/**
 * Decompress a file @a compressed_file_path produced by @ref v2f_file_compress_from_path,
 * using the compressor configuration given in @a header_file_path.
 * The reconstructed data are stored into @a reconstructed_file_path.
 *
 * The quantization and decorrelation parameters defined in @a header_file_path
 * can be overwritten using the parameters to this function.
 *
 * Please refer to the user manual for additional information on file formats
 * and other details.
 *
 * @param compressed_file_path path to the compressed bitstream to decompress.
 * @param header_file_path path to a copy of the header file used for
 *   compression.
 * @param reconstructed_file_path
 *
 * @param overwrite_quantizer_mode if true, the quantizer mode defined in
 *   @a header_file_path is overwritten.
 * @param quantizer_mode if @a overwrite_quantizer_mode is true, this mode
 *   is employed for dequantization. Otherwise, it is ignored.
 * @param overwrite_qstep if true, the quantizer step size defined in
 *   @a header_file_path is overwritten.
 * @param step_size if @a overwrite_qstep is true, and if the effective
 *   quantization mode is not NULL quantization, this is the step size
 *   employed for dequantization. Otherwise, it is ignored.
 * @param overwrite_decorrelator_mode if true, the decorrelator mode defined in
 *   @a header_file_path is overwritten.
 * @param decorrelator_mode if @a overwrite_decorrelator_mode is true,
 *   this is the decorrelation mode empoyed during decompression.
 *   Otherwise, it is ignored.
 * @param samples_per_row number of samples per row
 *
 * @return 0 if and only if decompression was successful.
 */
V2F_EXPORTED_SYMBOL
int v2f_file_decompress_from_path(
        char const *const compressed_file_path,
        char const *const header_file_path,
        char const *const reconstructed_file_path,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode,
        v2f_sample_t samples_per_row);

/**
 * Decompresses @a compressed_file into @a reconstructed_file
 * using the V2F codec defined in @a header_file.
 *
 * Please refer to the user manual for additional information on file formats
 * and other details.
 *
 * @param compressed_file file open for reading with the compressed data.
 * @param header_file file open for reading with the header data.
 * @param reconstructed_file file open for writing where the reconstructed
 *   data is to be written.
 *
 * @param overwrite_quantizer_mode if true, the quantizer mode defined in
 *   @a header_file_path is overwritten.
 * @param quantizer_mode if @a overwrite_quantizer_mode is true, this mode
 *   is employed for dequantization. Otherwise, it is ignored.
 * @param overwrite_qstep if true, the quantizer step size defined in
 *   @a header_file_path is overwritten.
 * @param step_size if @a overwrite_qstep is true, and if the effective
 *   quantization mode is not NULL quantization, this is the step size
 *   employed for dequantization. Otherwise, it is ignored.
 * @param overwrite_decorrelator_mode if true, the decorrelator mode defined in
 *   @a header_file_path is overwritten.
 * @param decorrelator_mode if @a overwrite_decorrelator_mode is true,
 *   this is the decorrelation mode empoyed during decompression.
 *   Otherwise, it is ignored.
 * @param samples_per_row number of samples per row
 *
 * @return 0 if and only if decompression was successful.
 */
V2F_EXPORTED_SYMBOL
int v2f_file_decompress_from_file(
        FILE *const compressed_file,
        FILE *const header_file,
        FILE *const reconstructed_file,
        bool overwrite_quantizer_mode,
        v2f_quantizer_mode_t quantizer_mode,
        bool overwrite_qstep,
        v2f_sample_t step_size,
        bool overwrite_decorrelator_mode,
        v2f_decorrelator_mode_t decorrelator_mode,
        v2f_sample_t samples_per_row);

#endif /* V2F_H */
