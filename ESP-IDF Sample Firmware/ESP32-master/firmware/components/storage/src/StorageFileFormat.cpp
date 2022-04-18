#include "StorageFileFormat.h"

namespace storage {

const char UNKNOWN_EXTENSION[] = "ext";
const char AR_EXTENSION[] = ".ar";
#ifdef ZIP_ARCHIVE
const char ZIP_EXTENSION[] = "zip";
#endif
#ifdef TAR_FORMAT
const char TAR_ZZ_EXTENSION[] = "tar.zz";
const char TAR_EXTENSION[] = "tar";
#endif
#ifdef MOLFAR_FORMAT
const char MOLFAR_ZZ_EXTENSION[] = "molfar.zz";
const char MOLFAR_EXTENSION[] = "molfar";
#endif
}  // namespace storage
