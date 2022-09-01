#ifndef _FUNCTIONGENERATOR_H_
#define _FUNCTIONGENERATOR_H_

void generator_init();
void pulseGenerator(volatile uint8_t *PORT, int frequeny);
void stairGenerator(volatile uint8_t *PORT, int frequeny);
void sineGenerator(volatile uint8_t *PORT, int frequeny);

#endif