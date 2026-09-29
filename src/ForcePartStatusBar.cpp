//
// This file is part of the aMule Project.
//
// Interactive part-status bar used by the amule-keroro forced-part dialog.
//

#include "ForcePartStatusBar.h"

#include <algorithm>

#include <wx/dcbuffer.h>
#include <wx/dcclient.h>
#include <wx/settings.h>

#include "BitVector.h"
#include "ClientRef.h"
#include "PartBarLegend.h"
#include "PartBarLegendUI.h"
#include "PartFile.h"
#include "Preferences.h"

namespace
{
void BuildLocalPartStatusSpec(CPartFile *file, CBarFillSpec &out)
{
	const uint16 partCount = file->GetPartCount();
	const uint64 fileSize = std::max<uint64>(1, file->GetFileSize());
	const bool flat = thePrefs::UseFlatBar();

	std::vector<CBarFillSpan> spans;
	spans.reserve(partCount);

	for (uint16 i = 0; i < partCount; ++i) {
		const uint64 start = PARTSIZE * i;
		const uint64 end = start + file->GetPartSize(i) - 1;
		const partbar::SourcePartState state =
			file->IsComplete(i) ? partbar::SourcePartState::Complete
					    : partbar::SourcePartState::Missing;
		spans.push_back({start, end, ToMuleColour(partbar::SourcePartColour(state, flat))});
	}

	out = CBarFillSpec(reinterpret_cast<wxUIntPtr>(file), fileSize, std::move(spans));
}

void BuildSourcePartStatusSpec(CPartFile *file, const CClientRef *source, CBarFillSpec &out)
{
	const uint16 partCount = file->GetPartCount();
	const uint64 fileSize = std::max<uint64>(1, file->GetFileSize());
	const bool flat = thePrefs::UseFlatBar();

	std::vector<CBarFillSpan> spans;
	spans.reserve(partCount);

	if (source == nullptr || !source->IsLinked()) {
		BuildLocalPartStatusSpec(file, out);
		return;
	}

	const BitVector &partStatus = source->GetPartStatus();
	const uint16 lastDownloadingPart =
		source->GetDownloadState() == DS_DOWNLOADING ? source->GetLastDownloadingPart() : 0xffff;
	const uint16 nextRequestedPart = source->GetNextRequestedPart();
	const bool stopped = file->IsStopped();

	if (partStatus.size() != partCount) {
		BuildLocalPartStatusSpec(file, out);
		return;
	}

	for (uint16 i = 0; i < partCount; ++i) {
		const uint64 start = PARTSIZE * i;
		const uint64 end = start + file->GetPartSize(i) - 1;

		partbar::SourcePartState state;
		if (!partStatus.get(i)) {
			state = partbar::SourcePartState::Missing;
		} else if (file->IsComplete(i)) {
			state = partbar::SourcePartState::Complete;
		} else if (lastDownloadingPart == i) {
			state = partbar::SourcePartState::Downloading;
		} else if (nextRequestedPart == i) {
			state = partbar::SourcePartState::NextRequested;
		} else {
			state = partbar::SourcePartState::Needed;
		}

		CMuleColour colour = ToMuleColour(partbar::SourcePartColour(state, flat));
		if (stopped) {
			colour.Blend(50);
		}
		spans.push_back({start, end, colour});
	}

	out = CBarFillSpec(reinterpret_cast<wxUIntPtr>(file), fileSize, std::move(spans));
}
} // namespace

wxBEGIN_EVENT_TABLE(CForcePartStatusBar, wxPanel)
	EVT_PAINT(CForcePartStatusBar::OnPaint)
	EVT_LEFT_DOWN(CForcePartStatusBar::OnLeftDown)
	EVT_MOTION(CForcePartStatusBar::OnMouseMove)
wxEND_EVENT_TABLE()

CForcePartStatusBar::CForcePartStatusBar(
	wxWindow *parent,
	CPartFile *file,
	const CClientRef *source)
: wxPanel(parent, wxID_ANY, wxDefaultPosition, wxSize(-1, 34), wxBORDER_NONE)
, m_file(file)
, m_source(source)
, m_partCount(file != nullptr ? file->GetPartCount() : 0)
, m_selectedPart(file != nullptr && file->GetPartCount() > 0 ? 0 : 0xffffffff)
, m_renderer()
, m_spec()
{
	SetBackgroundStyle(wxBG_STYLE_PAINT);
	RebuildSpec();
}

void CForcePartStatusBar::RebuildSpec()
{
	if (m_file == nullptr || m_partCount == 0) {
		m_spec = CBarFillSpec(reinterpret_cast<wxUIntPtr>(m_file), 1, {});
		return;
	}

	if (m_source != nullptr) {
		BuildSourcePartStatusSpec(m_file, m_source, m_spec);
	} else {
		BuildLocalPartStatusSpec(m_file, m_spec);
	}

	m_renderer.SetSpec(m_spec);
}

void CForcePartStatusBar::SelectFromX(int x)
{
	if (m_file == nullptr || m_partCount == 0) {
		return;
	}

	const int width = GetClientSize().GetWidth();
	if (width <= 0) {
		return;
	}

	x = std::max(0, std::min(x, width - 1));
	const uint64 fileSize = m_spec.GetFileSize();
	const uint64 offset = (static_cast<uint64>(x) * fileSize) / static_cast<uint64>(width);
	uint32 part = static_cast<uint32>(offset / PARTSIZE);
	if (part >= m_partCount) {
		part = m_partCount - 1;
	}

	if (part != m_selectedPart) {
		m_selectedPart = part;
		Refresh();
	}
}

void CForcePartStatusBar::OnPaint(wxPaintEvent &WXUNUSED(event))
{
	wxAutoBufferedPaintDC dc(this);
	dc.SetBackground(wxBrush(GetBackgroundColour()));
	dc.Clear();

	if (m_partCount == 0) {
		return;
	}

	wxRect cell = GetClientRect();
	m_renderer.Render(cell, &dc, 0);

	const int width = cell.GetWidth();
	const int height = cell.GetHeight();
	if (m_selectedPart < m_partCount && width > 0 && height > 4) {
		const uint64 fileSize = m_spec.GetFileSize();
		const uint64 partStart = static_cast<uint64>(m_selectedPart) * PARTSIZE;
		const uint64 partEnd =
			std::min<uint64>(fileSize, partStart + m_file->GetPartSize(m_selectedPart));

		int x1 = static_cast<int>((partStart * width) / fileSize);
		int x2 = static_cast<int>((partEnd * width) / fileSize);
		x1 = std::max(0, std::min(x1, width - 1));
		x2 = std::max(x1 + 1, std::min(x2, width));

		wxPen pen(ToMuleColour(partbar::kForcedSelection), 2);
		wxBrush brush(*wxTRANSPARENT_BRUSH);
		dc.SetPen(pen);
		dc.SetBrush(brush);
		dc.DrawRectangle(x1, 1, x2 - x1, height - 2);
	}
}

void CForcePartStatusBar::OnLeftDown(wxMouseEvent &event)
{
	SelectFromX(event.GetX());
	event.Skip();
}

void CForcePartStatusBar::OnMouseMove(wxMouseEvent &event)
{
	if (event.Dragging() && event.LeftIsDown()) {
		SelectFromX(event.GetX());
	}
	event.Skip();
}
