/**
 * @file v2f_file.h
 *
 * @brief Interface to handle files.
 */

#ifndef V2F_FILE_H
#define V2F_FILE_H

#include "v2f.h"
#include "v2f_compressor.h"
#include "v2f_decompressor.h"

/**
 * Write a compressor/decompressor pair to @a output_file with
 * the following format:
 *
 * - quantizer:
 *      - mode: 1 byte, must correspond to one of @ref v2f_quantizer_mode_t
 *      - step size: 4 bytes, unsigned big endian
 * - decorrelator:
 *      - mode: 2 bytes, unsigned big endian, must correspond to one
 *        of @ref v2f_decorrelator_mode_t
 *      - max_sample_value: 4 bytes, unsigned big endian. Must be at least
 *        as large as the maximum sample value for the V2F forest defined
 *        below, if present
 *
 * - forest_id: 4 bytes, unsigned bit endian.
 *   If this field is set to 0, it indicates that a explicit
 *   definition of the V2F forest is included afterwards.
 *   If set to a larger value v, the (v-1)-th prebuilt forest is used.
 *
 * - V2F forest: If forest_id != 0, an entropy coder/decoder pair definition,
 *   as output by @ref v2f_file_write_forest (variable length).
 *
 *
 * @param output_file file open for writing
 * @param compressor initialized compressor of the pair
 * @param decompressor initialized decompressor of the pair
 * @return
 *  - @ref V2F_E_NONE : Codec written successfull
 *  - @ref V2F_E_IO : Input/output error
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_file_write_codec(
        FILE* output_file,
        v2f_compressor_t *const compressor,
        v2f_decompressor_t *const decompressor);

/**
 * Read a compressor/decompressor pair from @a input_file.
 *
 * @param input_file file input for reading
 * @param compressor compressor to be initialized
 * @param decompressor decompressor to be initialized
 * @return
 *  - @ref V2F_E_NONE : Read successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_file_read_codec(
        FILE* input_file,
        v2f_compressor_t *const compressor,
        v2f_decompressor_t *const decompressor);


/**
 * Free all resources allocated when reading the compressor/decompressor
 * pair. Note that this function is only guaranteed to work if the
 * compressor and decompressor were produced by @ref v2f_file_read_codec.
 * @param compressor pointer to the compressor to destroy.
 * @param decompressor pointer to the decompressor to destroy
 * @return
 *  - @ref V2F_E_NONE : Destruction successfull
 *  - @ref V2F_E_INVALID_PARAMETER : At least one invalid parameter
 */
v2f_error_t v2f_file_destroy_read_codec(
        v2f_compressor_t *const compressor,
        v2f_decompressor_t *const decompressor);

/**
 * Write the configuration data needed to represent a coder.
 * A decoder can also be extracted from the output data.
 *
 * Format:
 *
 * - number of entries: total number of entries, counting those that do not
 *   have a word assigned and are not needed by the decoder,
 *   4 bytes, big endian, unsigned,
 *   @see V2F_C_MIN_ENTRY_COUNT, @see V2F_C_MAX_ENTRY_COUNT
 *
 * - bytes per word: number of bytes used to represent each of the nodes that
 *   are included in the V2F dictionary,
 *   1 byte, unsigned, @see V2F_C_MIN_BYTES_PER_WORD,
 *   @see V2F_C_MAX_BYTES_PER_WORD.
 *
 * - bytes per sample:  samples are represented with these many bytes,
 *   1 byte, unsigned, @see V2F_C_MIN_BYTES_PER_SAMPLE,
 *   @see V2F_C_MAX_BYTES_PER_SAMPLE
 *
 * - max expected value : max expected input sample value,
 *   2 bytes, big endian, unsigned, @see V2F_C_MAX_SAMPLE_VALUE,
 *   must be consistent with `bytes per sample`
 *
 * - root count : n-1, where n is the number of root nodes included,
 *   must be >= 1, 2 bytes BE unsigned, @see V2F_C_MAX_ROOT_COUNT,
 *   @see V2F_C_MAX_CHILD_COUNT.
 *
 * - For each root:
 *     - total entry count : total number of entries in this root, 4 bytes,
 *       big endian, @see V2F_C_MIN_ENTRY_COUNT, @see V2F_C_MAX_ENTRY_COUNT.
 *
 *     - number of included entries: total number of entries, excluding
 *       those that do not have a word assigned,
 *       4 bytes, big endian, unsigned.
 *       Must be consistent with the value of `bytes per word`.
 *
 *     - entries : variable length, `total entry count` elements (included
 *       those not included in the decoder dict) ordered by index
 *       For each entry in this root:
 *          - index: 4 bytes, big endian, unsigned.
 *            Even though this value can be deduced by the "ordered by index"
 *            constraint, this value is maintained to facilitate verification.
 *
 *          - number of children : 4 bytes, big endian, unsigned
 *            @see V2F_C_MAX_CHILD_COUNT
 *
 *        If `number of children` > 0, then the following items are also
 *        included for this entry of this root:
 *          - children indices: `number of children` * 4 bytes,
 *            ordered by corresponding sample index,
 *
 *        If `number of children` is not identical to
 *          `max expected value` + 1, then node is included in the V2F decoder
 *          and has an assigned codeword. In this case, the following
 *          fields are present:
 *          - sample count : number of samples represented by this index,
 *            2 bytes, big endian,
 *            @see V2F_C_MIN_SAMPLE_COUNT, @see V2F_C_MAX_SAMPLE_COUNT
 *
 *          - samples : `bytes per sample` * `sample count`
 *            sample values in `bytes per sample` bytes per sample,
 *            big endian, unsigned. Not present if `sample count` is zero.
 *
 *          - word : bytes corresponding to this included node,
 *            `bytes per word` bytes, big endian, unsigned
 *
 *     - number of children of the root node: 4 bytes, big endian, unsigned
 *       This number may not exceed the maximum number of possible sample
 *       values, but it may be lower than that. If it is, the number of
 *       children must be (possible values - i), where i is the index of
 *       this root. In this case, children must be assigned for input
 *       sample values from i onwards.
 *
 *     - indices of the children of the root node: indices of the roots's
 *       entries order by input symbol.
 *       For each children:
 *          - index: 4 bytes, unsigned big endian
 *          - input symbol value: `bytes per sample` bytes, unsigned big endian
 *
 *   Note that root count can be smaller than `max expected value`
 *   In that case, it is to be interpreted that all remaining
 *   roots are identical to the last one included.
 *
 * @param output file open for writing
 * @param coder coder of the coder/decoder pair.
 * @param decoder decoder of the coder/decoder pair.
 * @param different_roots only the first `different_roots` roots are saved
 *   by this function.
 *
 * @return
 *  - @ref V2F_E_NONE : Coder successfully dumped.
 *  - @ref V2F_E_INVALID_PARAMETER : Invalid input parameters.
 *  - @ref V2F_E_IO : I/O error saving the coder representation.
 */
v2f_error_t v2f_file_write_forest(FILE *output,
                                  v2f_entropy_coder_t const *const coder,
                                  v2f_entropy_decoder_t const *const decoder,
                                  uint32_t different_roots);

/**
 * Read a file containing the definition of a V2F forest.
 * Produce a coder and decoder pair.
 *
 * @param input file containing the definition,
 *   produced as defined in@ref v2f_file_write_forest.
 * @param coder pointer to the coder that will be initialized with the read
 *   data
 * @param decoder pointer to the decoder that will be initialized with
 *   the read data
 *
 * @return
 *  - @ref V2F_E_NONE : Coder successfully dumped.
 *  - @ref V2F_E_INVALID_PARAMETER : Invalid input parameters.
 *  - @ref V2F_E_OUT_OF_MEMORY : Memory could not be initialized for the
 *    decoder.
 *  - @ref V2F_E_IO : I/O error saving the coder representation.
 */
v2f_error_t v2f_file_read_forest(
        FILE *input,
        v2f_entropy_coder_t *const coder,
        v2f_entropy_decoder_t *const decoder);

/**
 * Free all memory allocated by @ref v2f_file_read_forest.
 * Freeing other coders/decoders may cause a crash.
 *
 * @param coder coder to be destroyed.
 * @param decoder decoder to be destroyed.
 *
 * @return
 *  - @ref V2F_E_NONE : Destruction successful
 *  - @ref V2F_E_INVALID_PARAMETER : Invalid coder or decoder
 */
v2f_error_t v2f_file_destroy_read_forest(v2f_entropy_coder_t *coder, v2f_entropy_decoder_t *decoder);

/**
 * Verify the validity of a coder/decoder pair. Useful when the pair
 * is loaded from a file.
 *
 * @param coder pointer to an initialized coder
 * @param decoder pointer to an initialized de coder
 *
 * @return
 *  - @ref V2F_E_NONE : Verification successful
 *  - @ref V2F_E_INVALID_PARAMETER : Invalid coder or decoder
 */
v2f_error_t v2f_verify_forest(
        v2f_entropy_coder_t *const coder, v2f_entropy_decoder_t *const decoder);

/**
 * Read up to @a max_sample_count samples in big endian format from
 * @a input_file, and store them into @a sample_buffer.
 * The @a bytes_per_sample parameter must be compatible with the definition
 * of the @ref v2f_sample_t type.
 *
 * Samples are read from @a input_file until either
 * @a max_sample_count samples have been read, or
 * until EOF is found. The destination buffer should
 * be ready to hold up to the maximum requested
 * number of samples.
 *
 * Note that if the number of samples obtained is less than @a max_sample_count,
 * then V2F_E_UNEXPECTED_END_OF_FILE is returned, and @a read_sample_count
 * is updated with the actual value. However, if the EOF is not aligned
 * with @a bytes_per_sample, V2F_E_IO is returned instead. Based on this,
 * attempting to read from an empty file or at SEEK_END will result
 * in returning V2F_E_UNEXPECTED_END_OF_FILE.
 *
 * @param input_file file open for reading.
 * @param sample_buffer buffer with space for at least `max_sample_count` elements.
 * @param max_sample_count maximum number of samples to be read.
 *   Cannot be larger than @ref V2F_C_MAX_BLOCK_SIZE.
 * @param bytes_per_sample number of bytes per sample used for reading
 * @param read_sample_count if not NULL, the total number of samples
 *   stored in @a sample_buffer is stored there. Otherwise, it is
 *   ignored.
 *
 * @return
 *  - @ref V2F_E_NONE : All requested samples were successfully read.
 *  - @ref V2F_E_UNEXPECTED_END_OF_FILE : Less than @a max_sample_count samples
 *    were copied to the buffer. The @a read_sample_count was updated
 *    with the actual count. The end of file occurred with correct alignment.
 *  - @ref V2F_E_INVALID_PARAMETER : Invalid input parameters.
 *  - @ref V2F_E_IO : An I/O error ocurred,
 *    or found EOF not aligned with @a bytes_per_sample,
 */
v2f_error_t v2f_file_read_big_endian(
        FILE *input_file,
        v2f_sample_t *sample_buffer,
        uint64_t max_sample_count,
        uint8_t bytes_per_sample,
        uint64_t* read_sample_count);

/**
 * Write samples to a file, storing them in big endian order.
 *
 * @param output_file file open for writing.
 * @param sample_buffer buffer of samples to be writen.
 * @param sample_count number of samples in the buffer.
 * @param bytes_per_sample number of bytes per output
 *
 * @return
 *  - @ref V2F_E_NONE : Samples successfully writen
 *  - @ref V2F_E_INVALID_PARAMETER : Invalid input parameters
 *  - @ref V2F_E_IO : Could not write the block
 */
v2f_error_t v2f_file_write_big_endian(
        FILE *output_file,
        v2f_sample_t *const sample_buffer,
        uint64_t sample_count,
        uint8_t bytes_per_sample);

#endif /* V2F_FILE_H */
