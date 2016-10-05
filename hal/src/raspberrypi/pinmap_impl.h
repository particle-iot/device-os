#ifndef PINMAP_IMPL_H
#define	PINMAP_IMPL_H

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct RPi_Pin_Info {
  bool pwm_capable;
  PinMode pin_mode;
} RPi_Pin_Info;

RPi_Pin_Info* HAL_Pin_Map(void);

#ifdef	__cplusplus
}
#endif

#endif	/* PINMAP_IMPL_H */

