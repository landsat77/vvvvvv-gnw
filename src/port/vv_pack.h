#ifndef VV_PACK_H
#define VV_PACK_H

#include <stdint.h>
#include <stdbool.h>

#define VV_PACK_MAGIC "GWVV01\0\0"

enum {
    VV_PACK_OK = 0,
    VV_PACK_ERR_MAGIC = -1,
    VV_PACK_ERR_CORRUPT = -2,
};

#ifdef __cplusplus
extern "C" {
#endif

int vv_pack_open(const uint8_t *data, uint32_t size);
bool vv_pack_find(const char *name, const uint8_t **data, uint32_t *size);
uint32_t vv_pack_count(void);
const char *vv_pack_name(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif
