//
// This file is part of the aMule Project.
//
// Temporary command-line restriction for downloading one part of one file.
//

#ifndef FORCE_PART_SELECTION_H
#define FORCE_PART_SELECTION_H

#include "MD4Hash.h"

namespace ForcePartSelection {

//! Select exactly one part of one file for download.
void Set(const CMD4Hash &fileHash, uint32 part);

//! Select exactly one part of one file for download from one specific source.
void SetSource(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash);

//! Disable the forced-part restriction.
void Clear();

//! Return whether a forced restriction is currently active.
bool IsActive();

//! Return whether the active restriction is tied to one source.
bool IsSourceRestricted();

//! Return the file hash targeted by the active restriction.
const CMD4Hash &GetFileHash();

//! Return the selected part of the active restriction.
uint32 GetPart();

//! Return the UserHash targeted by the active source restriction.
const CMD4Hash &GetSourceUserHash();

//! Return true when the given file/part is allowed to be requested.
bool IsAllowed(const CMD4Hash &fileHash, uint32 part);

//! Return true when the given file/part/source combination is allowed to be requested.
bool IsSourceAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash);

} // namespace ForcePartSelection

#endif // FORCE_PART_SELECTION_H
