#ifndef DOOM_PORT_STATUS_H_
#define DOOM_PORT_STATUS_H_

extern char doom_error_msg[256];

void doom_clear_error_msg(void);
void doom_set_error_literal(const char *msg);
void doom_set_error_msg(const char *fmt, ...);

#endif
