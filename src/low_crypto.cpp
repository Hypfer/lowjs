// -----------------------------------------------------------------------------
//  low_crypto.cpp
// -----------------------------------------------------------------------------

#include "low_crypto.h"
#include "LowCryptoHash.h"

#include "low_alloc.h"


// -----------------------------------------------------------------------------
//  low_crypto_create_hash
// -----------------------------------------------------------------------------

duk_ret_t low_crypto_create_hash(duk_context *ctx)
{
    low_t *low = duk_get_low_context(ctx);

    const char *type = duk_require_string(ctx, 1);

    int len = strlen(type);
    char *typeUpper = (char *)low_alloc_throw(ctx, len + 1);
    for(int i = 0; i < len; i++)
        typeUpper[i] = toupper((unsigned)type[i]);
    typeUpper[len] = 0;
    const mbedtls_md_info_t *info = mbedtls_md_info_from_string(typeUpper);
    low_free(typeUpper);

    if(!info)
        duk_reference_error(
          low->duk_ctx, "unsupported hashing algorithm %s!", type);

    unsigned char *key = NULL;
    duk_size_t key_len;
    if(!duk_is_undefined(ctx, 2))
    {
        key = (unsigned char *)duk_get_buffer_data(ctx, 2, &key_len);
        if(!key)
        {
            key = (unsigned char *)duk_require_string(ctx, 2);
            key_len = strlen((char *)key);
        }
    }

    LowCryptoHash *hash = new LowCryptoHash(low, info, key, key_len);

    int index;
    for(index = 0; index < low->cryptoHashes.size(); index++)
        if(!low->cryptoHashes[index])
        {
            low->cryptoHashes[index] = hash;
            break;
        }
    if(index == low->cryptoHashes.size())
        low->cryptoHashes.push_back(hash);
    hash->SetIndex(index);

    duk_push_int(low->duk_ctx, index);
    duk_push_c_function(ctx, low_crypto_hash_finalizer, 1);
    duk_set_finalizer(ctx, 0);

    return 1;
}

// -----------------------------------------------------------------------------
//  low_crypto_hash_finalizer
// -----------------------------------------------------------------------------

duk_ret_t low_crypto_hash_finalizer(duk_context *ctx)
{
    low_t *low = duk_get_low_context(ctx);

    duk_get_prop_string(ctx, 0, "_native");
    int index = duk_require_int(ctx, -1);

    if(index < 0 || index >= low->cryptoHashes.size())
        duk_reference_error(ctx, "crypto hash not found");

    delete low->cryptoHashes[index];
    return 0;
}


// -----------------------------------------------------------------------------
//  low_crypto_hash_update
// -----------------------------------------------------------------------------

duk_ret_t low_crypto_hash_update(duk_context *ctx)
{
    low_t *low = duk_get_low_context(ctx);

    int index = duk_require_int(ctx, 0);
    if(index < 0 || index >= low->cryptoHashes.size())
        duk_reference_error(ctx, "crypto hash not found");

    duk_size_t len;
    auto buffer = duk_require_buffer_data(ctx, 1, &len);

    low->cryptoHashes[index]->Update((unsigned char *)buffer, len);
    return 0;
}


// -----------------------------------------------------------------------------
//  low_crypto_hash_digest
// -----------------------------------------------------------------------------

duk_ret_t low_crypto_hash_digest(duk_context *ctx)
{
    low_t *low = duk_get_low_context(ctx);

    int index = duk_require_int(ctx, 0);
    if(index < 0 || index >= low->cryptoHashes.size())
        duk_reference_error(ctx, "crypto hash not found");

    int len = low->cryptoHashes[index]->OutputSize();
    auto buffer = low_push_buffer(ctx, len);

    low->cryptoHashes[index]->Digest((unsigned char *)buffer, len);
    return 1;
}


// -----------------------------------------------------------------------------
//  low_crypto_random_bytes
// -----------------------------------------------------------------------------

duk_ret_t low_crypto_random_bytes(duk_context *ctx)
{
    int len = duk_require_int(ctx, 0);
    if(!duk_is_undefined(ctx, 1))
        duk_reference_error(ctx, "crypto.randomBytes async version not implemented yet");

    unsigned char *buffer = (unsigned char *)low_push_buffer(ctx, len);
    for(int i = 0; i < len; i++)
        buffer[i] = rand();

    return 1;
}
