//
// This file is part of the aMule Project.
//
// GUI for selecting a forced download part, optionally restricted to one source.
//

#ifndef FORCE_PART_DIALOG_H
#define FORCE_PART_DIALOG_H

#include "MD4Hash.h"

#include <wx/string.h>

#include <wx/dialog.h>

class CPartFile;
class CClientRef;
class CForcePartStatusBar;

class CForcePartDialog : public wxDialog
{
public:
	CForcePartDialog(wxWindow *parent, CPartFile *file, const CClientRef *source = nullptr);

	bool WasAccepted() const { return m_accepted; }
	uint32 GetPart() const;
	const CMD4Hash &GetFileHash() const { return m_fileHash; }

private:
	CMD4Hash m_fileHash;
	wxString m_fileName;
	uint16 m_partCount;
	CMD4Hash m_sourceHash;
	bool m_hasSource;
	bool m_accepted;
	CForcePartStatusBar *m_partStatusBar;

	void OnAccept(wxCommandEvent &event);
	void OnCancel(wxCommandEvent &event);

	wxDECLARE_EVENT_TABLE();
};

#endif // FORCE_PART_DIALOG_H
