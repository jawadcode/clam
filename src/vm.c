#include <string.h>

#include "vm.h"

bool VM_Value_eq(VM_Value a, VM_Value b) {
    if (a.tag != b.tag)
        return false;
    else
        switch (a.tag) {
        case VM_VALUE_UNIT:
            return true;
        case VM_VALUE_BOOL:
            return a.value.boolean == b.value.boolean;
        case VM_VALUE_NUMBER:
            return a.value.number == b.value.number;
        case VM_VALUE_STRING:
            return String_eq(a.value.string, b.value.string);
        case VM_VALUE_LIST: {
            ValueVec a_list = a.value.list;
            ValueVec b_list = b.value.list;
            if (a_list.length != b_list.length)
                return false;
            else {
                for (size_t i = 0; i < a_list.length; i++) {
                    if (!VM_Value_eq(a_list.buffer[i], b_list.buffer[i]))
                        return false;
                }
                return true;
            }
        }
        case VM_FUNCTION:
            return a.value.fun == b.value.fun;
        }
}