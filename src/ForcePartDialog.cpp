//
// This file is part of the aMule Project.
//
// GUI for selecting a forced download part, optionally restricted to one source.
//

#include "ForcePartDialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include "ClientRef.h"
#include "ForcePartStatusBar.h"
#include "DownloadQueue.h"
#include "amule.h"
#include "ForcePartSelection.h"
#include "PartFile.h"
#include "PartBarLegendUI.h"
#include "Preferences.h"

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
, m_relaySequence(nullptr)
, m_relayFinalPartFirst(nullptr)
, m_relayPingPong(nullptr)
, m_originHashText(nullptr)
, m_originChoice(nullptr)
, m_nicknameBaseText(nullptr)
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
			_("Select the actual file part on the Part Status bar below. The selected physical part "
			  "is the part shown in the file's part map.")),
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

	m_relaySequence = new wxCheckBox(this, wxID_ANY, _("Configure a sequential relay stage"));
	mainSizer->Add(m_relaySequence, 0, wxLEFT | wxRIGHT | wxTOP, 12);
	m_relayFinalPartFirst = new wxCheckBox(this, wxID_ANY,
		_("Download the final part first, then part 0, part 1, and so on"));
	mainSizer->Add(m_relayFinalPartFirst, 0, wxLEFT | wxRIGHT | wxTOP, 12);
	mainSizer->Add(new wxStaticText(this, wxID_ANY,
		_("When enabled, nickname suffixes identify relay order steps; the part bar still shows file parts.")),
		0, wxLEFT | wxRIGHT | wxTOP, 12);
	mainSizer->Add(new wxStaticText(this, wxID_ANY,
		_("The selected download supplies the file hash. Enter source S's UserHash and one shared "
		  "nickname base. Neighboring daemon names are derived automatically.")),
		0, wxLEFT | wxRIGHT | wxTOP, 12);
	m_relayPingPong = new wxCheckBox(this, wxID_ANY,
		_("Use two-node ping-pong mode (alternate relay work between two daemons)"));
	mainSizer->Add(m_relayPingPong, 0, wxLEFT | wxRIGHT | wxTOP, 12);
	wxFlexGridSizer *relayGrid = new wxFlexGridSizer(2, 6, 6);
	relayGrid->AddGrowableCol(1, 1);
	auto addRelayField = [&](const wxString &label, wxTextCtrl *&control, const wxString &value) {
		relayGrid->Add(new wxStaticText(this, wxID_ANY, label), 0, wxALIGN_CENTER_VERTICAL);
		control = new wxTextCtrl(this, wxID_ANY, value);
		relayGrid->Add(control, 1, wxEXPAND);
	};
	addRelayField(_("Source S UserHash:"), m_originHashText,
		m_hasSource ? m_sourceHash.Encode() : wxString());
	relayGrid->Add(new wxStaticText(this, wxID_ANY, _("Known source:")), 0, wxALIGN_CENTER_VERTICAL);
	m_originChoice = new wxChoice(this, wxID_ANY);
	std::map<CMD4Hash, wxString> knownSources;
	for (const CClientRef &candidate : file->GetSourceList()) {
		if (!candidate.IsLinked()) {
			continue;
		}
		const CMD4Hash userHash = candidate.GetUserHash();
		const wxString nick = candidate.GetUserName();
		if (userHash.IsEmpty() || nick.IsEmpty()) {
			continue;
		}
		knownSources[userHash] = nick;
	}
	int selectedOrigin = wxNOT_FOUND;
	for (const auto &entry : knownSources) {
		const wxString hash = entry.first.Encode();
		const wxString label = entry.second + " — " + hash.Left(8);
		m_originChoice->Append(label);
		m_originHashes.push_back(entry.first);
		if (entry.first == m_sourceHash) {
			selectedOrigin = static_cast<int>(m_originHashes.size()) - 1;
		}
	}
	if (selectedOrigin != wxNOT_FOUND) {
		m_originChoice->SetSelection(selectedOrigin);
	}
	m_originChoice->Bind(wxEVT_CHOICE, &CForcePartDialog::OnOriginChoice, this);
	relayGrid->Add(m_originChoice, 1, wxEXPAND);
	addRelayField(_("Shared nickname base:"), m_nicknameBaseText, wxEmptyString);
	mainSizer->Add(relayGrid, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

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

	if (m_relaySequence->GetValue()) {
		CMD4Hash originHash;
		const wxString originText = m_originHashText->GetValue().Strip(wxString::both);
		const wxString nicknameBase = m_nicknameBaseText->GetValue().Strip(wxString::both);
		const bool pingPong = m_relayPingPong->GetValue();
		const bool finalPartFirst = m_relayFinalPartFirst->GetValue();
		const uint32 selectedFilePart = GetPart();
		const uint32 sequencePart = finalPartFirst
			? ForcePartSelection::GetSequenceStepForFilePart(
				m_partCount, selectedFilePart, finalPartFirst)
			: selectedFilePart;
		const uint32 nodeIndex = pingPong ? sequencePart % 2 : sequencePart;
		const wxString expectedLocalName = nicknameBase + wxString::Format("-%u", nodeIndex);
		if (!originHash.Decode(originText) || nicknameBase.IsEmpty() ||
			(pingPong && sequencePart > 1)) {
			wxMessageBox(_("Enter a valid source S UserHash and nickname base. Ping-pong mode "
				"must start at schedule step 0 or 1. With final-part-first enabled, select the "
				"final part for step 0 or part 0 for step 1; otherwise select part 0 or part 1."),
				_("Relay sequence"), wxOK | wxICON_WARNING, this);
			return;
		}
		if (thePrefs::GetUserNick() != expectedLocalName) {
			thePrefs::SetUserNick(expectedLocalName);
			if (theApp->glob_prefs != nullptr) {
				theApp->glob_prefs->Save();
			}
		}
		ForcePartSelection::SetSequenceByBase(
			m_fileHash, sequencePart, originHash, nicknameBase, pingPong, finalPartFirst);
		ForcePartSelection::ReportSequenceProgress(sequencePart == 0
			? wxString(CFormat(finalPartFirst
				? "Configured as '%s'; downloading the final part first from source S"
				: "Configured as '%s'; downloading part 0 first from source S") %
				expectedLocalName)
			: wxString(CFormat("Configured as '%s'; searching for upstream '%s'") %
				expectedLocalName % (nicknameBase + wxString::Format("-%u", 1 - nodeIndex))));
	} else if (m_hasSource) {
		ForcePartSelection::SetSource(m_fileHash, GetPart(), m_sourceHash);
	} else {
		ForcePartSelection::Set(m_fileHash, GetPart());
	}

	// The restriction is now active. Rebuild any existing download request pipeline
	// immediately instead of requiring a manual Stop/Resume cycle.
	file->ApplyForcedPartSelection();
	if (file->IsAutoDownPriority()) {
		file->SetAutoDownPriority(false);
	}
	if (file->GetDownPriority() != PR_HIGH) {
		file->SetDownPriority(PR_HIGH);
	}

	m_accepted = true;
	EndModal(wxID_OK);
}

void CForcePartDialog::OnOriginChoice(wxCommandEvent &WXUNUSED(event))
{
	const int selection = m_originChoice->GetSelection();
	if (selection >= 0 && static_cast<size_t>(selection) < m_originHashes.size()) {
		m_originHashText->SetValue(m_originHashes[selection].Encode());
	}
}

void CForcePartDialog::OnCancel(wxCommandEvent &WXUNUSED(event))
{
	EndModal(wxID_CANCEL);
}
