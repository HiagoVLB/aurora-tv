#pragma once

typedef struct ResBundleC ResBundleC;
ResBundleC *resBundle_createWithRootPath(const char *locale, const char *file, const char *root);
const char *resBundle_getLocString(ResBundleC *bundle, const char *msgid);
void resBundle_destroy(ResBundleC *bundle);
