/*
 * omphalos.h — provisor omphalos: interfacies publica
 *
 * Hoc caput declarat solum quae omphalos.c exportat:
 * provisorem et functionem nominis.
 *
 * Pro typis et functionibus transformatoris ipsius,
 * include omphalos/ὀμφαλός.h et omphalos/λεκτήρ.h directe.
 */

#ifndef OMPHALOS_H
#define OMPHALOS_H

#include "provisor.h"

extern const provisor_t provisor_omphalos;

void omphalos_pone_nomen(const char *nomen);

#endif /* OMPHALOS_H */
