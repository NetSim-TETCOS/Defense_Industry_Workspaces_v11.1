#include "main.h"
#include "Ethernet.h"
#include "Ethernet_enum.h"
#undef _NETSIM_ETHERNET_ENUM_H_
#define GENERATE_ENUM_STRINGS
#include "Ethernet_enum.h"
#undef GENERATE_ENUM_STRINGS
