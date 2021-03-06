#include "kal.h"

extern kal_loglevel_t debug_level_flag;

static const char *ll2str[] =
{
    "FATAL",
    "ERROR",
    "WARN ",
    "INFO ",
    "DEBUG"
};

char kal_print_buf[1024];
char kal_print_line[512];
char kal_log_buf[1024];
char kal_log_line[512];

int kal_log(int log_level,
            const char *file,
            int line,
            const char *fmt,
            ...)
{
    int i = 0;
    int j = 0;
    va_list ap;

    if (log_level < KAL_LL_FATAL)
        log_level = KAL_LL_FATAL;
    else if (log_level > KAL_LL_DEBUG)
        log_level = KAL_LL_DEBUG;

    if (log_level < KAL_LL_INFO)
        kal_printf("%s %s:%d: ", ll2str[log_level], file, line);

    va_start(ap, fmt);
    vsnprintf(kal_log_buf, 1023,fmt, ap);
    va_end(ap);

    i = 0;
    while(i<1023 &&  j<512 && kal_log_buf[i] != '\0')
    {
        switch(kal_log_buf[i])
        {
            case '\n':
                kal_log_line[j] = '\0';
               printf("%s\r\n", kal_log_line);
                j = 0;
                break;
            default:
                kal_log_line[j] = kal_log_buf[i];
                j++;
                break;
        }
        i++;
    }
    if(kal_log_buf[i] == '\0')
    {
        kal_log_line[j] = '\0';
         printf("%s", kal_log_line);
    }

    if (log_level < KAL_LL_INFO)
        kal_printf("\n");

    return 0;
}

int kal_print_func(const char *fmt,  ...)
{
    int i = 0;
    int j = 0;
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(kal_print_buf, 1023,fmt, ap);
    va_end(ap);

    i = 0;
    while(i<1023 &&  j<512 && kal_print_buf[i] != '\0')
    {
        switch(kal_print_buf[i])
        {
            case '\n':
                kal_print_line[j] = '\0';
               printf("%s\r\n", kal_print_line);
                j = 0;
                break;
            default:
                kal_print_line[j] = kal_print_buf[i];
                j++;
                break;
        }
        i++;
    }
    if(kal_print_buf[i] == '\0')
    {
        kal_print_line[j] = '\0';
         printf("%s", kal_print_line);
    }
    return 0;
}

