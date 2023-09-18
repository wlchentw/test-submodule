#ifndef ntx_charger_misc_h //[
#define ntx_charger_misc_h

#include <linux/power/ntx_charger_type.h>


typedef int (*ntx_charger_get_type)(void);

int ntx_charger_register_cc_detector(ntx_charger_get_type cc_ctype_cb);
int ntx_charger_register_gadget_detector(ntx_charger_get_type gadget_ctype_cb);

int ntx_charger_cc_detect(int iTimeout_ms);
int ntx_charger_gadget_detect(int iTimeout_ms);



#endif //] ntx_charger_misc_h

