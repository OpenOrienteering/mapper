/* Copyright (C) 2001-2005 Peter Selinger.
   This file is part of potrace. It is free software and it is covered
   by the GNU General Public License. See the file COPYING for details. */

#include <stdlib.h>
#include <string.h>

#include "potracelib.h"
#include "curve.h"
#include "trace.h"
#include "progress.h"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* default parameters */
static const potrace_param_t param_default = {
  2,                             /* turdsize */
  POTRACE_TURNPOLICY_MINORITY,   /* turnpolicy */
  1.0,                           /* alphamax */
  1,                             /* opticurve */
  0.2,                           /* opttolerance */
  {
    NULL,                        /* callback function */
    NULL,                        /* callback data */
    0.0, 1.0,                    /* progress range */
    0.0,                         /* granularity */
  },
};

/* Return a fresh copy of the set of default parameters, or NULL on
   failure with errno set. */
potrace_param_t *potrace_param_default() {
  potrace_param_t *p;

  p = (potrace_param_t *) malloc(sizeof(potrace_param_t));
  if (!p) {
    return NULL;
  }
  memcpy(p, &param_default, sizeof(potrace_param_t));
  return p;
}
