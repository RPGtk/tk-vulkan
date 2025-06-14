#ifndef TKVUL_MAIN_H
#define TKVUL_MAIN_H

typedef enum
{
    TKVUL_NO_ERROR
} tkvul_error_t;

tkvul_error_t tkvul_create(void);
void tkvul_destroy(void);

#endif // TKVUL_MAIN_H
