#ifndef VERSION_H
#define VERSION_H

// =============================================================================
// FILEMASTER - Version Management System
// =============================================================================

// Version numbers - EDIT THESE TO RELEASE NEW VERSION
#define VERSION_MAJOR    2
#define VERSION_MINOR    0
#define VERSION_PATCH    0
#define VERSION_BUILD    0

// Automatically generated strings
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define VERSION_STRING TOSTRING(VERSION_MAJOR) "." TOSTRING(VERSION_MINOR) "." TOSTRING(VERSION_PATCH) "." TOSTRING(VERSION_BUILD)
#define VERSION_RC VERSION_MAJOR,VERSION_MINOR,VERSION_PATCH,VERSION_BUILD

// Build information
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Product information
#define INTERNAL_NAME       "FileMaster"

#endif // VERSION_H