#pragma once

namespace storage {

enum class FileFormat {
    UNKNOWN,
    AR,
#ifdef ZIP_ARCHIVE
    ZIP,
#endif
#ifdef TAR_FORMAT
    TAR_ZZ,
    TAR,
#endif
#ifdef MOLFAR_FORMAT
    MOLFAR_ZZ,
    MOLFAR
#endif
};

extern const char UNKNOWN_EXTENSION[];
extern const char AR_EXTENSION[];
#ifdef ZIP_ARCHIVE
extern const char ZIP_EXTENSION[];
#endif
#ifdef TAR_FORMAT
extern const char TAR_ZZ_EXTENSION[];
extern const char TAR_EXTENSION[];
#endif
#ifdef MOLFAR_FORMAT
extern const char MOLFAR_ZZ_EXTENSION[];
extern const char MOLFAR_EXTENSION[];
#endif
}  // namespace storage
