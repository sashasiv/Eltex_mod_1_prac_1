// msg.c
#include "msg.h"

const char* msg_type_name(int type) {
    switch (type) {
    case MSG_READY:  return "READY";
    case MSG_HEADER: return "HEADER";
    case MSG_DATA:   return "DATA";
    case MSG_END:    return "END";
    case MSG_DONE:   return "DONE";
    default:         return "UNKNOWN";
    }
}