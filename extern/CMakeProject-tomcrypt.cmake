set(TOMCRYPT_SRC "libtomcrypt/src/ciphers/anubis.c"
                 "libtomcrypt/src/ciphers/blowfish.c"
                 "libtomcrypt/src/ciphers/camellia.c"
                 "libtomcrypt/src/ciphers/cast5.c"
                 "libtomcrypt/src/ciphers/des.c"
                 "libtomcrypt/src/ciphers/kasumi.c"
                 "libtomcrypt/src/ciphers/khazad.c"
                 "libtomcrypt/src/ciphers/kseed.c"
                 "libtomcrypt/src/ciphers/multi2.c"
                 "libtomcrypt/src/ciphers/noekeon.c"
                 "libtomcrypt/src/ciphers/rc2.c"
                 "libtomcrypt/src/ciphers/rc5.c"
                 "libtomcrypt/src/ciphers/rc6.c"
                 "libtomcrypt/src/ciphers/skipjack.c"
                 "libtomcrypt/src/ciphers/xtea.c"
                 "libtomcrypt/src/ciphers/aes/aes.c"
                 "libtomcrypt/src/ciphers/safer/safer.c"
                 "libtomcrypt/src/ciphers/safer/saferp.c"
                 "libtomcrypt/src/ciphers/twofish/twofish.c"
                 "libtomcrypt/src/encauth/ccm/ccm_add_aad.c"
                 "libtomcrypt/src/encauth/ccm/ccm_add_nonce.c"
                 "libtomcrypt/src/encauth/ccm/ccm_done.c"
                 "libtomcrypt/src/encauth/ccm/ccm_init.c"
                 "libtomcrypt/src/encauth/ccm/ccm_memory.c"
                 "libtomcrypt/src/encauth/ccm/ccm_process.c"
                 "libtomcrypt/src/encauth/ccm/ccm_reset.c"
                 "libtomcrypt/src/encauth/ccm/ccm_test.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_add_aad.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_decrypt.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_done.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_encrypt.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_init.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_memory.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_setiv.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_setiv_rfc7905.c"
                 "libtomcrypt/src/encauth/chachapoly/chacha20poly1305_test.c"
                 "libtomcrypt/src/encauth/eax/eax_addheader.c"
                 "libtomcrypt/src/encauth/eax/eax_decrypt.c"
                 "libtomcrypt/src/encauth/eax/eax_decrypt_verify_memory.c"
                 "libtomcrypt/src/encauth/eax/eax_done.c"
                 "libtomcrypt/src/encauth/eax/eax_encrypt.c"
                 "libtomcrypt/src/encauth/eax/eax_encrypt_authenticate_memory.c"
                 "libtomcrypt/src/encauth/eax/eax_init.c"
                 "libtomcrypt/src/encauth/eax/eax_test.c"
                 "libtomcrypt/src/encauth/gcm/gcm_add_aad.c"
                 "libtomcrypt/src/encauth/gcm/gcm_add_iv.c"
                 "libtomcrypt/src/encauth/gcm/gcm_done.c"
                 "libtomcrypt/src/encauth/gcm/gcm_gf_mult.c"
                 "libtomcrypt/src/encauth/gcm/gcm_init.c"
                 "libtomcrypt/src/encauth/gcm/gcm_memory.c"
                 "libtomcrypt/src/encauth/gcm/gcm_mult_h.c"
                 "libtomcrypt/src/encauth/gcm/gcm_process.c"
                 "libtomcrypt/src/encauth/gcm/gcm_reset.c"
                 "libtomcrypt/src/encauth/gcm/gcm_test.c"
                 "libtomcrypt/src/encauth/ocb/ocb_decrypt.c"
                 "libtomcrypt/src/encauth/ocb/ocb_decrypt_verify_memory.c"
                 "libtomcrypt/src/encauth/ocb/ocb_done_decrypt.c"
                 "libtomcrypt/src/encauth/ocb/ocb_done_encrypt.c"
                 "libtomcrypt/src/encauth/ocb/ocb_encrypt.c"
                 "libtomcrypt/src/encauth/ocb/ocb_encrypt_authenticate_memory.c"
                 "libtomcrypt/src/encauth/ocb/ocb_init.c"
                 "libtomcrypt/src/encauth/ocb/ocb_ntz.c"
                 "libtomcrypt/src/encauth/ocb/ocb_shift_xor.c"
                 "libtomcrypt/src/encauth/ocb/ocb_test.c"
                 "libtomcrypt/src/encauth/ocb/s_ocb_done.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_add_aad.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_decrypt.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_decrypt_last.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_decrypt_verify_memory.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_done.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_encrypt.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_encrypt_authenticate_memory.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_encrypt_last.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_init.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_int_ntz.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_int_xor_blocks.c"
                 "libtomcrypt/src/encauth/ocb3/ocb3_test.c"
                 "libtomcrypt/src/hashes/blake2b.c"
                 "libtomcrypt/src/hashes/blake2s.c"
                 "libtomcrypt/src/hashes/md2.c"
                 "libtomcrypt/src/hashes/md4.c"
                 "libtomcrypt/src/hashes/md5.c"
                 "libtomcrypt/src/hashes/rmd128.c"
                 "libtomcrypt/src/hashes/rmd160.c"
                 "libtomcrypt/src/hashes/rmd256.c"
                 "libtomcrypt/src/hashes/rmd320.c"
                 "libtomcrypt/src/hashes/sha1.c"
                 "libtomcrypt/src/hashes/sha3.c"
                 "libtomcrypt/src/hashes/sha3_test.c"
                 "libtomcrypt/src/hashes/tiger.c"
                 "libtomcrypt/src/hashes/chc/chc.c"
                 "libtomcrypt/src/hashes/helper/hash_file.c"
                 "libtomcrypt/src/hashes/helper/hash_filehandle.c"
                 "libtomcrypt/src/hashes/helper/hash_memory.c"
                 "libtomcrypt/src/hashes/helper/hash_memory_multi.c"
                 "libtomcrypt/src/hashes/sha2/sha224.c"
                 "libtomcrypt/src/hashes/sha2/sha256.c"
                 "libtomcrypt/src/hashes/sha2/sha384.c"
                 "libtomcrypt/src/hashes/sha2/sha512.c"
                 "libtomcrypt/src/hashes/sha2/sha512_224.c"
                 "libtomcrypt/src/hashes/sha2/sha512_256.c"
                 "libtomcrypt/src/hashes/whirl/whirl.c"
                 "libtomcrypt/src/mac/blake2/blake2bmac.c"
                 "libtomcrypt/src/mac/blake2/blake2bmac_file.c"
                 "libtomcrypt/src/mac/blake2/blake2bmac_memory.c"
                 "libtomcrypt/src/mac/blake2/blake2bmac_memory_multi.c"
                 "libtomcrypt/src/mac/blake2/blake2bmac_test.c"
                 "libtomcrypt/src/mac/blake2/blake2smac.c"
                 "libtomcrypt/src/mac/blake2/blake2smac_file.c"
                 "libtomcrypt/src/mac/blake2/blake2smac_memory.c"
                 "libtomcrypt/src/mac/blake2/blake2smac_memory_multi.c"
                 "libtomcrypt/src/mac/blake2/blake2smac_test.c"
                 "libtomcrypt/src/mac/f9/f9_done.c"
                 "libtomcrypt/src/mac/f9/f9_file.c"
                 "libtomcrypt/src/mac/f9/f9_init.c"
                 "libtomcrypt/src/mac/f9/f9_memory.c"
                 "libtomcrypt/src/mac/f9/f9_memory_multi.c"
                 "libtomcrypt/src/mac/f9/f9_process.c"
                 "libtomcrypt/src/mac/f9/f9_test.c"
                 "libtomcrypt/src/mac/hmac/hmac_done.c"
                 "libtomcrypt/src/mac/hmac/hmac_file.c"
                 "libtomcrypt/src/mac/hmac/hmac_init.c"
                 "libtomcrypt/src/mac/hmac/hmac_memory.c"
                 "libtomcrypt/src/mac/hmac/hmac_memory_multi.c"
                 "libtomcrypt/src/mac/hmac/hmac_process.c"
                 "libtomcrypt/src/mac/hmac/hmac_test.c"
                 "libtomcrypt/src/mac/omac/omac_done.c"
                 "libtomcrypt/src/mac/omac/omac_file.c"
                 "libtomcrypt/src/mac/omac/omac_init.c"
                 "libtomcrypt/src/mac/omac/omac_memory.c"
                 "libtomcrypt/src/mac/omac/omac_memory_multi.c"
                 "libtomcrypt/src/mac/omac/omac_process.c"
                 "libtomcrypt/src/mac/omac/omac_test.c"
                 "libtomcrypt/src/mac/pelican/pelican.c"
                 "libtomcrypt/src/mac/pelican/pelican_memory.c"
                 "libtomcrypt/src/mac/pelican/pelican_test.c"
                 "libtomcrypt/src/mac/pmac/pmac_done.c"
                 "libtomcrypt/src/mac/pmac/pmac_file.c"
                 "libtomcrypt/src/mac/pmac/pmac_init.c"
                 "libtomcrypt/src/mac/pmac/pmac_memory.c"
                 "libtomcrypt/src/mac/pmac/pmac_memory_multi.c"
                 "libtomcrypt/src/mac/pmac/pmac_ntz.c"
                 "libtomcrypt/src/mac/pmac/pmac_process.c"
                 "libtomcrypt/src/mac/pmac/pmac_shift_xor.c"
                 "libtomcrypt/src/mac/pmac/pmac_test.c"
                 "libtomcrypt/src/mac/poly1305/poly1305.c"
                 "libtomcrypt/src/mac/poly1305/poly1305_file.c"
                 "libtomcrypt/src/mac/poly1305/poly1305_memory.c"
                 "libtomcrypt/src/mac/poly1305/poly1305_memory_multi.c"
                 "libtomcrypt/src/mac/poly1305/poly1305_test.c"
                 "libtomcrypt/src/mac/xcbc/xcbc_done.c"
                 "libtomcrypt/src/mac/xcbc/xcbc_file.c"
                 "libtomcrypt/src/mac/xcbc/xcbc_init.c"
                 "libtomcrypt/src/mac/xcbc/xcbc_memory.c"
                 "libtomcrypt/src/mac/xcbc/xcbc_memory_multi.c"
                 "libtomcrypt/src/mac/xcbc/xcbc_process.c"
                 "libtomcrypt/src/mac/xcbc/xcbc_test.c"
                 "libtomcrypt/src/math/gmp_desc.c"
                 "libtomcrypt/src/math/ltm_desc.c"
                 "libtomcrypt/src/math/multi.c"
                 "libtomcrypt/src/math/radix_to_bin.c"
                 "libtomcrypt/src/math/rand_bn.c"
                 "libtomcrypt/src/math/rand_prime.c"
                 "libtomcrypt/src/math/tfm_desc.c"
                 "libtomcrypt/src/math/fp/ltc_ecc_fp_mulmod.c"
                 "libtomcrypt/src/misc/adler32.c"
                 "libtomcrypt/src/misc/burn_stack.c"
                 "libtomcrypt/src/misc/compare_testvector.c"
                 "libtomcrypt/src/misc/crc32.c"
                 "libtomcrypt/src/misc/error_to_string.c"
                 "libtomcrypt/src/misc/mem_neq.c"
                 "libtomcrypt/src/misc/pk_get_oid.c"
                 "libtomcrypt/src/misc/zeromem.c"
                 "libtomcrypt/src/misc/base64/base64_decode.c"
                 "libtomcrypt/src/misc/base64/base64_encode.c"
                 "libtomcrypt/src/misc/crypt/crypt.c"
                 "libtomcrypt/src/misc/crypt/crypt_argchk.c"
                 "libtomcrypt/src/misc/crypt/crypt_cipher_descriptor.c"
                 "libtomcrypt/src/misc/crypt/crypt_cipher_is_valid.c"
                 "libtomcrypt/src/misc/crypt/crypt_constants.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_cipher.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_cipher_any.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_cipher_id.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_hash.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_hash_any.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_hash_id.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_hash_oid.c"
                 "libtomcrypt/src/misc/crypt/crypt_find_prng.c"
                 "libtomcrypt/src/misc/crypt/crypt_fsa.c"
                 "libtomcrypt/src/misc/crypt/crypt_hash_descriptor.c"
                 "libtomcrypt/src/misc/crypt/crypt_hash_is_valid.c"
                 "libtomcrypt/src/misc/crypt/crypt_inits.c"
                 "libtomcrypt/src/misc/crypt/crypt_ltc_mp_descriptor.c"
                 "libtomcrypt/src/misc/crypt/crypt_prng_descriptor.c"
                 "libtomcrypt/src/misc/crypt/crypt_prng_is_valid.c"
                 "libtomcrypt/src/misc/crypt/crypt_prng_rng_descriptor.c"
                 "libtomcrypt/src/misc/crypt/crypt_register_all_ciphers.c"
                 "libtomcrypt/src/misc/crypt/crypt_register_all_hashes.c"
                 "libtomcrypt/src/misc/crypt/crypt_register_all_prngs.c"
                 "libtomcrypt/src/misc/crypt/crypt_register_cipher.c"
                 "libtomcrypt/src/misc/crypt/crypt_register_hash.c"
                 "libtomcrypt/src/misc/crypt/crypt_register_prng.c"
                 "libtomcrypt/src/misc/crypt/crypt_sizes.c"
                 "libtomcrypt/src/misc/crypt/crypt_unregister_cipher.c"
                 "libtomcrypt/src/misc/crypt/crypt_unregister_hash.c"
                 "libtomcrypt/src/misc/crypt/crypt_unregister_prng.c"
                 "libtomcrypt/src/misc/hkdf/hkdf.c"
                 "libtomcrypt/src/misc/hkdf/hkdf_test.c"
                 "libtomcrypt/src/misc/pkcs5/pkcs_5_1.c"
                 "libtomcrypt/src/misc/pkcs5/pkcs_5_2.c"
                 "libtomcrypt/src/misc/pkcs5/pkcs_5_test.c"
                 "libtomcrypt/src/modes/cbc/cbc_decrypt.c"
                 "libtomcrypt/src/modes/cbc/cbc_done.c"
                 "libtomcrypt/src/modes/cbc/cbc_encrypt.c"
                 "libtomcrypt/src/modes/cbc/cbc_getiv.c"
                 "libtomcrypt/src/modes/cbc/cbc_setiv.c"
                 "libtomcrypt/src/modes/cbc/cbc_start.c"
                 "libtomcrypt/src/modes/cfb/cfb_decrypt.c"
                 "libtomcrypt/src/modes/cfb/cfb_done.c"
                 "libtomcrypt/src/modes/cfb/cfb_encrypt.c"
                 "libtomcrypt/src/modes/cfb/cfb_getiv.c"
                 "libtomcrypt/src/modes/cfb/cfb_setiv.c"
                 "libtomcrypt/src/modes/cfb/cfb_start.c"
                 "libtomcrypt/src/modes/ctr/ctr_decrypt.c"
                 "libtomcrypt/src/modes/ctr/ctr_done.c"
                 "libtomcrypt/src/modes/ctr/ctr_encrypt.c"
                 "libtomcrypt/src/modes/ctr/ctr_getiv.c"
                 "libtomcrypt/src/modes/ctr/ctr_setiv.c"
                 "libtomcrypt/src/modes/ctr/ctr_start.c"
                 "libtomcrypt/src/modes/ctr/ctr_test.c"
                 "libtomcrypt/src/modes/ecb/ecb_decrypt.c"
                 "libtomcrypt/src/modes/ecb/ecb_done.c"
                 "libtomcrypt/src/modes/ecb/ecb_encrypt.c"
                 "libtomcrypt/src/modes/ecb/ecb_start.c"
                 "libtomcrypt/src/modes/f8/f8_decrypt.c"
                 "libtomcrypt/src/modes/f8/f8_done.c"
                 "libtomcrypt/src/modes/f8/f8_encrypt.c"
                 "libtomcrypt/src/modes/f8/f8_getiv.c"
                 "libtomcrypt/src/modes/f8/f8_setiv.c"
                 "libtomcrypt/src/modes/f8/f8_start.c"
                 "libtomcrypt/src/modes/f8/f8_test_mode.c"
                 "libtomcrypt/src/modes/lrw/lrw_decrypt.c"
                 "libtomcrypt/src/modes/lrw/lrw_done.c"
                 "libtomcrypt/src/modes/lrw/lrw_encrypt.c"
                 "libtomcrypt/src/modes/lrw/lrw_getiv.c"
                 "libtomcrypt/src/modes/lrw/lrw_process.c"
                 "libtomcrypt/src/modes/lrw/lrw_setiv.c"
                 "libtomcrypt/src/modes/lrw/lrw_start.c"
                 "libtomcrypt/src/modes/lrw/lrw_test.c"
                 "libtomcrypt/src/modes/ofb/ofb_decrypt.c"
                 "libtomcrypt/src/modes/ofb/ofb_done.c"
                 "libtomcrypt/src/modes/ofb/ofb_encrypt.c"
                 "libtomcrypt/src/modes/ofb/ofb_getiv.c"
                 "libtomcrypt/src/modes/ofb/ofb_setiv.c"
                 "libtomcrypt/src/modes/ofb/ofb_start.c"
                 "libtomcrypt/src/modes/xts/xts_decrypt.c"
                 "libtomcrypt/src/modes/xts/xts_done.c"
                 "libtomcrypt/src/modes/xts/xts_encrypt.c"
                 "libtomcrypt/src/modes/xts/xts_init.c"
                 "libtomcrypt/src/modes/xts/xts_mult_x.c"
                 "libtomcrypt/src/modes/xts/xts_test.c"
                 "libtomcrypt/src/pk/asn1/der/bit/der_decode_bit_string.c"
                 "libtomcrypt/src/pk/asn1/der/bit/der_decode_raw_bit_string.c"
                 "libtomcrypt/src/pk/asn1/der/bit/der_encode_bit_string.c"
                 "libtomcrypt/src/pk/asn1/der/bit/der_encode_raw_bit_string.c"
                 "libtomcrypt/src/pk/asn1/der/bit/der_length_bit_string.c"
                 "libtomcrypt/src/pk/asn1/der/boolean/der_decode_boolean.c"
                 "libtomcrypt/src/pk/asn1/der/boolean/der_encode_boolean.c"
                 "libtomcrypt/src/pk/asn1/der/boolean/der_length_boolean.c"
                 "libtomcrypt/src/pk/asn1/der/choice/der_decode_choice.c"
                 "libtomcrypt/src/pk/asn1/der/generalizedtime/der_decode_generalizedtime.c"
                 "libtomcrypt/src/pk/asn1/der/generalizedtime/der_encode_generalizedtime.c"
                 "libtomcrypt/src/pk/asn1/der/generalizedtime/der_length_generalizedtime.c"
                 "libtomcrypt/src/pk/asn1/der/ia5/der_decode_ia5_string.c"
                 "libtomcrypt/src/pk/asn1/der/ia5/der_encode_ia5_string.c"
                 "libtomcrypt/src/pk/asn1/der/ia5/der_length_ia5_string.c"
                 "libtomcrypt/src/pk/asn1/der/integer/der_decode_integer.c"
                 "libtomcrypt/src/pk/asn1/der/integer/der_encode_integer.c"
                 "libtomcrypt/src/pk/asn1/der/integer/der_length_integer.c"
                 "libtomcrypt/src/pk/asn1/der/object_identifier/der_decode_object_identifier.c"
                 "libtomcrypt/src/pk/asn1/der/object_identifier/der_encode_object_identifier.c"
                 "libtomcrypt/src/pk/asn1/der/object_identifier/der_length_object_identifier.c"
                 "libtomcrypt/src/pk/asn1/der/octet/der_decode_octet_string.c"
                 "libtomcrypt/src/pk/asn1/der/octet/der_encode_octet_string.c"
                 "libtomcrypt/src/pk/asn1/der/octet/der_length_octet_string.c"
                 "libtomcrypt/src/pk/asn1/der/printable_string/der_decode_printable_string.c"
                 "libtomcrypt/src/pk/asn1/der/printable_string/der_encode_printable_string.c"
                 "libtomcrypt/src/pk/asn1/der/printable_string/der_length_printable_string.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_ex.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_flexi.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_decode_sequence_multi.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_decode_subject_public_key_info.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_encode_sequence_ex.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_encode_sequence_multi.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_encode_subject_public_key_info.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_length_sequence.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_sequence_free.c"
                 "libtomcrypt/src/pk/asn1/der/sequence/der_sequence_shrink.c"
                 "libtomcrypt/src/pk/asn1/der/set/der_encode_set.c"
                 "libtomcrypt/src/pk/asn1/der/set/der_encode_setof.c"
                 "libtomcrypt/src/pk/asn1/der/short_integer/der_decode_short_integer.c"
                 "libtomcrypt/src/pk/asn1/der/short_integer/der_encode_short_integer.c"
                 "libtomcrypt/src/pk/asn1/der/short_integer/der_length_short_integer.c"
                 "libtomcrypt/src/pk/asn1/der/teletex_string/der_decode_teletex_string.c"
                 "libtomcrypt/src/pk/asn1/der/teletex_string/der_length_teletex_string.c"
                 "libtomcrypt/src/pk/asn1/der/utctime/der_decode_utctime.c"
                 "libtomcrypt/src/pk/asn1/der/utctime/der_encode_utctime.c"
                 "libtomcrypt/src/pk/asn1/der/utctime/der_length_utctime.c"
                 "libtomcrypt/src/pk/asn1/der/utf8/der_decode_utf8_string.c"
                 "libtomcrypt/src/pk/asn1/der/utf8/der_encode_utf8_string.c"
                 "libtomcrypt/src/pk/asn1/der/utf8/der_length_utf8_string.c"
                 "libtomcrypt/src/pk/dh/dh.c"
                 "libtomcrypt/src/pk/dh/dh_check_pubkey.c"
                 "libtomcrypt/src/pk/dh/dh_export.c"
                 "libtomcrypt/src/pk/dh/dh_export_key.c"
                 "libtomcrypt/src/pk/dh/dh_free.c"
                 "libtomcrypt/src/pk/dh/dh_generate_key.c"
                 "libtomcrypt/src/pk/dh/dh_import.c"
                 "libtomcrypt/src/pk/dh/dh_set.c"
                 "libtomcrypt/src/pk/dh/dh_set_pg_dhparam.c"
                 "libtomcrypt/src/pk/dh/dh_shared_secret.c"
                 "libtomcrypt/src/pk/dsa/dsa_decrypt_key.c"
                 "libtomcrypt/src/pk/dsa/dsa_encrypt_key.c"
                 "libtomcrypt/src/pk/dsa/dsa_export.c"
                 "libtomcrypt/src/pk/dsa/dsa_free.c"
                 "libtomcrypt/src/pk/dsa/dsa_generate_key.c"
                 "libtomcrypt/src/pk/dsa/dsa_generate_pqg.c"
                 "libtomcrypt/src/pk/dsa/dsa_import.c"
                 "libtomcrypt/src/pk/dsa/dsa_make_key.c"
                 "libtomcrypt/src/pk/dsa/dsa_set.c"
                 "libtomcrypt/src/pk/dsa/dsa_set_pqg_dsaparam.c"
                 "libtomcrypt/src/pk/dsa/dsa_shared_secret.c"
                 "libtomcrypt/src/pk/dsa/dsa_sign_hash.c"
                 "libtomcrypt/src/pk/dsa/dsa_verify_hash.c"
                 "libtomcrypt/src/pk/dsa/dsa_verify_key.c"
                 "libtomcrypt/src/pk/ecc/ecc.c"
                 "libtomcrypt/src/pk/ecc/ecc_ansi_x963_export.c"
                 "libtomcrypt/src/pk/ecc/ecc_ansi_x963_import.c"
                 "libtomcrypt/src/pk/ecc/ecc_decrypt_key.c"
                 "libtomcrypt/src/pk/ecc/ecc_encrypt_key.c"
                 "libtomcrypt/src/pk/ecc/ecc_export.c"
                 "libtomcrypt/src/pk/ecc/ecc_free.c"
                 "libtomcrypt/src/pk/ecc/ecc_get_size.c"
                 "libtomcrypt/src/pk/ecc/ecc_import.c"
                 "libtomcrypt/src/pk/ecc/ecc_make_key.c"
                 "libtomcrypt/src/pk/ecc/ecc_shared_secret.c"
                 "libtomcrypt/src/pk/ecc/ecc_sign_hash.c"
                 "libtomcrypt/src/pk/ecc/ecc_sizes.c"
                 "libtomcrypt/src/pk/ecc/ecc_test.c"
                 "libtomcrypt/src/pk/ecc/ecc_verify_hash.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_is_valid_idx.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_map.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_mul2add.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_mulmod.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_mulmod_timing.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_points.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_projective_add_point.c"
                 "libtomcrypt/src/pk/ecc/ltc_ecc_projective_dbl_point.c"
                 "libtomcrypt/src/pk/katja/katja_decrypt_key.c"
                 "libtomcrypt/src/pk/katja/katja_encrypt_key.c"
                 "libtomcrypt/src/pk/katja/katja_export.c"
                 "libtomcrypt/src/pk/katja/katja_exptmod.c"
                 "libtomcrypt/src/pk/katja/katja_free.c"
                 "libtomcrypt/src/pk/katja/katja_import.c"
                 "libtomcrypt/src/pk/katja/katja_make_key.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_i2osp.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_mgf1.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_decode.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_oaep_encode.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_os2ip.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_pss_decode.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_pss_encode.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_decode.c"
                 "libtomcrypt/src/pk/pkcs1/pkcs_1_v1_5_encode.c"
                 "libtomcrypt/src/pk/rsa/rsa_decrypt_key.c"
                 "libtomcrypt/src/pk/rsa/rsa_encrypt_key.c"
                 "libtomcrypt/src/pk/rsa/rsa_export.c"
                 "libtomcrypt/src/pk/rsa/rsa_exptmod.c"
                 "libtomcrypt/src/pk/rsa/rsa_free.c"
                 "libtomcrypt/src/pk/rsa/rsa_get_size.c"
                 "libtomcrypt/src/pk/rsa/rsa_import.c"
                 "libtomcrypt/src/pk/rsa/rsa_import_pkcs8.c"
                 "libtomcrypt/src/pk/rsa/rsa_import_x509.c"
                 "libtomcrypt/src/pk/rsa/rsa_make_key.c"
                 "libtomcrypt/src/pk/rsa/rsa_set.c"
                 "libtomcrypt/src/pk/rsa/rsa_sign_hash.c"
                 "libtomcrypt/src/pk/rsa/rsa_sign_saltlen_get.c"
                 "libtomcrypt/src/pk/rsa/rsa_verify_hash.c"
                 "libtomcrypt/src/prngs/chacha20.c"
                 "libtomcrypt/src/prngs/fortuna.c"
                 "libtomcrypt/src/prngs/rc4.c"
                 "libtomcrypt/src/prngs/rng_get_bytes.c"
                 "libtomcrypt/src/prngs/rng_make_prng.c"
                 "libtomcrypt/src/prngs/sober128.c"
                 "libtomcrypt/src/prngs/sprng.c"
                 "libtomcrypt/src/prngs/yarrow.c"
                 "libtomcrypt/src/stream/chacha/chacha_crypt.c"
                 "libtomcrypt/src/stream/chacha/chacha_done.c"
                 "libtomcrypt/src/stream/chacha/chacha_ivctr32.c"
                 "libtomcrypt/src/stream/chacha/chacha_ivctr64.c"
                 "libtomcrypt/src/stream/chacha/chacha_keystream.c"
                 "libtomcrypt/src/stream/chacha/chacha_setup.c"
                 "libtomcrypt/src/stream/chacha/chacha_test.c"
                 "libtomcrypt/src/stream/rc4/rc4_stream.c"
                 "libtomcrypt/src/stream/rc4/rc4_test.c"
                 "libtomcrypt/src/stream/sober128/sober128_stream.c"
                 "libtomcrypt/src/stream/sober128/sober128_test.c")

set(TOMCRYPT_HPP "libtomcrypt/src/headers/tomcrypt.h"
                 "libtomcrypt/src/headers/tomcrypt_argchk.h"
                 "libtomcrypt/src/headers/tomcrypt_cfg.h"
                 "libtomcrypt/src/headers/tomcrypt_cipher.h"
                 "libtomcrypt/src/headers/tomcrypt_custom.h"
                 "libtomcrypt/src/headers/tomcrypt_hash.h"
                 "libtomcrypt/src/headers/tomcrypt_mac.h"
                 "libtomcrypt/src/headers/tomcrypt_macros.h"
                 "libtomcrypt/src/headers/tomcrypt_math.h"
                 "libtomcrypt/src/headers/tomcrypt_misc.h"
                 "libtomcrypt/src/headers/tomcrypt_pk.h"
                 "libtomcrypt/src/headers/tomcrypt_pkcs.h"
                 "libtomcrypt/src/headers/tomcrypt_prng.h")

source_group("" FILES ${TOMCRYPT_SRC} ${TOMCRYPT_HPP})

add_library("tomcrypt" STATIC ${TOMCRYPT_SRC} ${TOMCRYPT_HPP})

set_property(TARGET "tomcrypt" PROPERTY FOLDER "External Libraries")

# Required since building from the source.
target_compile_definitions("tomcrypt" PRIVATE LTC_SOURCE)

# Required since tommath is a dependency.
target_compile_definitions("tomcrypt" PUBLIC LTM_DESC)

# This was defined behind an always active block.
target_compile_definitions("tomcrypt" PRIVATE LTC_DEVRANDOM)

# Common formulas used by our app.
target_compile_definitions("tomcrypt" PRIVATE LTC_SHA256)
target_compile_definitions("tomcrypt" PRIVATE LTC_SHA1)
target_compile_definitions("tomcrypt" PRIVATE LTC_MD5)

# Use the full AES encryption items.
target_compile_definitions("tomcrypt" PRIVATE LTC_YARROW)
target_compile_definitions("tomcrypt" PRIVATE LTC_YARROW_AES=3)

# Other definitions we used in the past, but whose meanings are not clear.
target_compile_definitions("tomcrypt" PRIVATE LTC_NO_PKCS)
target_compile_definitions("tomcrypt" PRIVATE LTC_PKCS_1)
target_compile_definitions("tomcrypt" PRIVATE LTC_DER)
target_compile_definitions("tomcrypt" PRIVATE LTC_NO_MODES)
target_compile_definitions("tomcrypt" PRIVATE LTC_ECB_MODE)
target_compile_definitions("tomcrypt" PRIVATE LTC_CBC_MODE)
target_compile_definitions("tomcrypt" PRIVATE LTC_CTR_MODE)
target_compile_definitions("tomcrypt" PRIVATE LTC_NO_HASHES)
target_compile_definitions("tomcrypt" PRIVATE LTC_NO_MACS) # no MAC (message authentication code) support
target_compile_definitions("tomcrypt" PRIVATE LTC_NO_PRNGS)
target_compile_definitions("tomcrypt" PRIVATE LTC_RNG_GET_BYTES)
target_compile_definitions("tomcrypt" PRIVATE LTC_RNG_MAKE_PRNG)
target_compile_definitions("tomcrypt" PRIVATE LTC_TRY_URANDOM_FIRST)
target_compile_definitions("tomcrypt" PRIVATE LTC_NO_PK)
target_compile_definitions("tomcrypt" PRIVATE LTC_MRSA)
target_compile_definitions("tomcrypt" PRIVATE LTC_NO_PROTOTYPES)

if(WITH_PORTABLE_TOMCRYPT)
  target_compile_definitions("tomcrypt" PRIVATE LTC_NO_ASM)
endif()
if(WITH_NO_ROLC_TOMCRYPT)
  target_compile_definitions("tomcrypt" PRIVATE LTC_NO_ROLC)
endif()

# Fixes build failures due to clang warnings
# See SM5-LTS/SM5-LTS#107 for details
# TODO(teejusb/natano): Remove these two lines once these issues have been fixed upstream.
set_property(TARGET "tomcrypt" PROPERTY C_STANDARD 90)
set_property(TARGET "tomcrypt" PROPERTY C_STANDARD_REQUIRED ON)
if(APPLE)
  set_property(TARGET "tomcrypt" PROPERTY XCODE_ATTRIBUTE_GCC_NO_COMMON_BLOCKS "YES")
endif()
if(MSVC)
  target_compile_definitions("tomcrypt" PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

disable_project_warnings("tomcrypt")

target_link_libraries("tomcrypt" "tommath")

target_include_directories("tomcrypt" PUBLIC "libtomcrypt/src/headers")
