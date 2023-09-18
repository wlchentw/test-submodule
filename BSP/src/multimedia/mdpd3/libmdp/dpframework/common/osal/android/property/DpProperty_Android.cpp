#include "DpProperty.h"
#if CONFIG_FOR_PROPERTY_SUPPORT
#include <cutils/properties.h>
#endif

DP_STATUS_ENUM getProperty(const char *pName, int32_t *pProp)
{
#if CONFIG_FOR_PROPERTY_SUPPORT
    char value[PROPERTY_VALUE_MAX];

    memset(value, 0x0, sizeof(value));
    property_get(pName, value, "0");
    *pProp = atoi(value);
#endif
    return DP_STATUS_RETURN_SUCCESS;
}

DP_STATUS_ENUM getProperty(const char *pName, char *pProp)
{
#if CONFIG_FOR_PROPERTY_SUPPORT
    char value[PROPERTY_VALUE_MAX];

    memset(value, 0x0, sizeof(value));
    property_get(pName, value, "");
    sprintf(pProp, "%s", value);
#endif
    return DP_STATUS_RETURN_SUCCESS;
}
