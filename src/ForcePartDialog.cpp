//
// This file is part of the aMule Project.
//
// GUI for selecting a forced download part, optionally restricted to one source.
//

#include "ForcePartDialog.h"

#include <wx/button.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#include "ClientRef.h"
#include "ForcePartStatusBar.h"
#include "DownloadQueue.h"
#include "amule.h"
#include "ForcePartSelection.h"
#include "PartFile.h"
#include "PartBarLegendUI.h"

wxBEGIN_EVENT_TABLE(CForcePartDialog, wxDialog)
	EVT_BUTTON(wxID_OK, CForcePartDialog::OnAccept)
	EVT_BUTTON(wxID_CANCEL, CForcePartDialog::OnCancel)
wxEND_EVENT_TABLE()

CForcePartDialog::CForcePartDialog(wxWindow *parent, CPartFile *file, const CClientRef *source)
: wxDialog(parent,
	wxID_ANY,
	_("Force download part"),
	wxDefaultPosition,
	wxDefaultSize,
	wxDEFAULT_DIALOG_STYLE)
, m_fileHash(file->GetFileHash())
, m_fileName(file->GetFileName().GetPrintable())
, m_partCount(file->GetPartCount())
, m_sourceHash()
, m_hasSource(false)
, m_accepted(false)
, m_partStatusBar(nullptr)
{
	wxASSERT(file != nullptr);

	if (source != nullptr && source->IsLinked()) {
		m_sourceHash = source->GetUserHash();
		m_hasSource = !m_sourceHash.IsEmpty();
	}
	SetTitle(m_hasSource ? _("Force download part from source") : _("Force download part"));

	wxBoxSizer *mainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer *grid = new wxFlexGridSizer(2, 8, 8);
	grid->AddGrowableCol(1, 1);

	grid->Add(new wxStaticText(this, wxID_ANY, _("File:")), 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(new wxStaticText(this, wxID_ANY, m_fileName),
		1, wxEXPAND);

	grid->Add(new wxStaticText(this, wxID_ANY, _("File hash:")), 0, wxALIGN_CENTER_VERTICAL);
	grid->Add(new wxStaticText(this, wxID_ANY, m_fileHash.Encode()),
		1, wxEXPAND);

	if (m_hasSource) {
		grid->Add(new wxStaticText(this, wxID_ANY, _("Source UserHash:")),
			0, wxALIGN_CENTER_VERTICAL);
		grid->Add(new wxStaticText(this, wxID_ANY, m_sourceHash.Encode()), 1, wxEXPAND);
	}

	mainSizer->Add(grid, wxSizerFlags(0).Expand().Border(wxTOP | wxLEFT | wxRIGHT, 12));

	mainSizer->Add(
		new wxStaticText(
			this,
			wxID_ANY,
			_("Select the part directly on the Part Status bar below:")),
		0,
		wxLEFT | wxRIGHT | wxTOP,
		12);

	m_partStatusBar = new CForcePartStatusBar(this, file, m_hasSource ? source : nullptr);
	mainSizer->Add(m_partStatusBar, 0, wxLEFT | wxRIGHT | wxTOP | wxEXPAND, 12);

	// Keep the selection cue visually tied to the existing Part Status palette.
	wxFlexGridSizer *legend = new wxFlexGridSizer(2, 6, 6);
	legend->Add(
		new wxStaticBitmap(this, wxID_ANY, MakeLegendSwatch(partbar::kForcedSelection)),
		0,
		wxALIGN_CENTER_VERTICAL);
	legend->Add(
		new wxStaticText(this, wxID_ANY, _("Orange = part selected for forced download")),
		0,
		wxALIGN_CENTER_VERTICAL);
	mainSizer->Add(legend, 0, wxLEFT | wxRIGHT | wxTOP, 12);

	wxString mode;
	if (m_hasSource) {
		mode = _("Only this part will be requested, and only from this source.");
	} else {
		mode = _("Only this part will be requested. Other parts will not be downloaded.");
	}
	mainSizer->Add(new wxStaticText(this, wxID_ANY, mode), 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

	wxBoxSizer *buttons = new wxBoxSizer(wxHORIZONTAL);
	buttons->AddStretchSpacer(1);
	buttons->Add(new wxButton(this, wxID_CANCEL, _("Cancel")), 0, wxRIGHT, 6);
	buttons->Add(new wxButton(this, wxID_OK, _("Force")), 0);
	mainSizer->Add(buttons, 0, wxLEFT | wxRIGHT | wxBOTTOM | wxEXPAND, 12);

	SetSizerAndFit(mainSizer);
	CentreOnParent();

	if (m_partCount == 0) {
		m_partStatusBar->Disable();
	}
}

uint32 CForcePartDialog::GetPart() const
{
	return m_partStatusBar != nullptr ? m_partStatusBar->GetSelectedPart() : 0;
}

void CForcePartDialog::OnAccept(wxCommandEvent &WXUNUSED(event))
{
	if (m_partCount == 0) {
		return;
	}

	CPartFile *file = theApp->downloadqueue->GetFileByID(m_fileHash);
	if (file == nullptr || file->GetPartCount() == 0) {
		wxMessageBox(_("The selected file is no longer in the download queue."),
			_("Force download part"), wxOK | wxICON_WARNING, this);
		return;
	}

	if (m_hasSource) {
		ForcePartSelection::SetSource(m_fileHash, GetPart(), m_sourceHash);
	} else {
		ForcePartSelection::Set(m_fileHash, GetPart());
	}

	// The restriction is now active. Rebuild any existing download request pipeline
	// immediately instead of requiring a manual Stop/Resume cycle.
	file->ApplyForcedPartSelection();

	m_accepted = true;
	EndModal(wxID_OK);
}

void CForcePartDialog::OnCancel(wxCommandEvent &WXUNUSED(event))
{
	EndModal(wxID_CANCEL);
}
