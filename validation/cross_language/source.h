#ifndef VALIDATION_SOURCE_PROVIDER_H
#define VALIDATION_SOURCE_PROVIDER_H
#include "tetrodotoxin/language/provider.h"
TTX_EXTERN_C tetrodotoxin_dialect_provider validation_source_provider(void);
TTX_EXTERN_C uint64_t validation_source_live_graphs(void);
#endif
