//
// This file is part of the aMule Project.
//
// Interactive part-status bar used by the amule-keroro forced-part dialog.
//

#ifndef FORCE_PART_STATUS_BAR_H
#define FORCE_PART_STATUS_BAR_H

#include <wx/panel.h>

#include "MuleBarRenderer.h"

class CClientRef;
class CPartFile;
class wxMouseEvent;
class wxPaintEvent;

class CForcePartStatusBar : public wxPanel
{
public:
	CForcePartStatusBar(
		wxWindow *parent,
		CPartFile *file,
		const CClientRef *source = nullptr);

	uint32 GetSelectedPart() const { return m_selectedPart; }
	bool HasSelection() const { return m_selectedPart < m_partCount; }

private:
	CPartFile *m_file;
	const CClientRef *m_source;
	uint16 m_partCount;
	uint32 m_selectedPart;
	CMuleBarRenderer m_renderer;
	CBarFillSpec m_spec;

	void RebuildSpec();
	void SelectFromX(int x);

	void OnPaint(wxPaintEvent &event);
	void OnLeftDown(wxMouseEvent &event);
	void OnMouseMove(wxMouseEvent &event);

	wxDECLARE_EVENT_TABLE();
};

#endif // FORCE_PART_STATUS_BAR_H
