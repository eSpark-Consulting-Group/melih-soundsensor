#ifndef LWIP_HDR_APPS_SNTP_H
#define LWIP_HDR_APPS_SNTP_H

#ifdef __cplusplus
extern "C" {
#endif

typedef uint8_t u8_t;

/* SNTP operating modes: default is to poll using unicast.
   The mode has to be set before calling sntp_init(). */
#define SNTP_OPMODE_POLL 0
#define SNTP_OPMODE_LISTENONLY 1

void sntp_init( void );
void sntp_setoperatingmode( u8_t operating_mode );
void sntp_setservername( u8_t idx, char* server );

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_APPS_SNTP_H */
