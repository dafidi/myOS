#ifndef __ERROR_H__
#define __ERROR_H__

enum sys_error {
    NONE,
    DISK_ERR_CANT_COMPLETE_READ_NOT_ENOUGH_DATA,
};

#define SPIN_ON(cond)\
if (cond) {\
    ERR_INFO(cond);\
}

#define ERR_INFO(x) \
print_string(__FILE__"(");\
print_int32(__LINE__);\
print_string(") [E]: ");\
print_string(#x);\
while(1)

#endif /* __ERROR_H__ */
