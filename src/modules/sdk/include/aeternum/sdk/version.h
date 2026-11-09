/*
 * Aeternum Module SDK — version macros (v0 DRAFT, not frozen).
 * M7: static lifecycle formalized; majors may still move without deprecation.
 */
#ifndef AETERNUM_SDK_VERSION_H
#define AETERNUM_SDK_VERSION_H

#define AETERNUM_SDK_VERSION_MAJOR 0
#define AETERNUM_SDK_VERSION_MINOR 2
#define AETERNUM_SDK_VERSION_PATCH 0
#define AETERNUM_SDK_VERSION \
    ((AETERNUM_SDK_VERSION_MAJOR * 10000) + (AETERNUM_SDK_VERSION_MINOR * 100) + AETERNUM_SDK_VERSION_PATCH)

/* Draft tag — remove when 1.0.0 is declared stable. */
#define AETERNUM_SDK_API_DRAFT 1

#endif
