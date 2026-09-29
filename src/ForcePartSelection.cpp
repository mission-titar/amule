//
// This file is part of the aMule Project.
//
// Temporary command-line restriction for downloading one part of one file.
//

#include "ForcePartSelection.h"

namespace {
CMD4Hash s_fileHash;
CMD4Hash s_userHash;
uint32 s_part = 0;
bool s_active = false;
bool s_source_active = false;
} // namespace

namespace ForcePartSelection {

void Set(const CMD4Hash &fileHash, uint32 part)
{
	s_fileHash = fileHash;
	s_userHash.Clear();
	s_part = part;
	s_active = true;
	s_source_active = false;
}

void SetSource(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash)
{
	s_fileHash = fileHash;
	s_userHash = userHash;
	s_part = part;
	s_active = true;
	s_source_active = true;
}

void Clear()
{
	s_fileHash.Clear();
	s_userHash.Clear();
	s_part = 0;
	s_active = false;
	s_source_active = false;
}

bool IsActive()
{
	return s_active;
}

bool IsSourceRestricted()
{
	return s_active && s_source_active;
}

const CMD4Hash &GetFileHash()
{
	return s_fileHash;
}

uint32 GetPart()
{
	return s_part;
}

const CMD4Hash &GetSourceUserHash()
{
	return s_userHash;
}

bool IsAllowed(const CMD4Hash &fileHash, uint32 part)
{
	return !s_active || (fileHash == s_fileHash && part == s_part);
}

bool IsSourceAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash)
{
	return !s_source_active ||
		(fileHash == s_fileHash && part == s_part && userHash == s_userHash);
}

} // namespace ForcePartSelection
