// IDE-only shims for clangd to understand CEdev-specific integer widths.
// Not compiled into the ez80 build; used via -include for clangd only.
#ifndef CLANGD_EZCXX_COMPAT_H
#define CLANGD_EZCXX_COMPAT_H

// Provide 48-bit width gate used by CE headers
#ifndef __SIZEOF_INT48__
#define __SIZEOF_INT48__ 6
#endif

// Map ez80 token to a host-parsable type so headers parse under clangd
#ifndef __int48
#define __int48 long long
#endif

// Provide 24-bit typedef gates used by CE headers if missing
#ifndef __INT24_TYPE__
#define __INT24_TYPE__ int
#endif
#ifndef __UINT24_TYPE__
#define __UINT24_TYPE__ unsigned int
#endif

#endif // CLANGD_EZCXX_COMPAT_H

