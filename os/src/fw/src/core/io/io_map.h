/* Copyright (c) 2016 Qualcomm Technologies International, Ltd. */
/*   %%version */
/**
 * /file
 *
 *  Select the right io_map.h based on the chip target
 */

/*lint --e{451} Double inclusion protection intentionally absent */



#  ifdef USE_IO_MAP_PUBLIC
#   include "io/auraplus/d00/io/io_map_public.h"
#  else
#   include "io/auraplus/d00/io/io_map.h"
#  endif /* USE_IO_MAP_PUBLIC */

