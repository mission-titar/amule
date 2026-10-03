//
// This file is part of the aMule Project.
//
// Local per-part source/lock selection for Keroro.
//

#ifndef FORCE_PART_SELECTION_H
#define FORCE_PART_SELECTION_H

#include "MD4Hash.h"
#include <wx/string.h>

namespace ForcePartSelection {

    //! Select exactly one part of one file for download (legacy API).
    void Set(const CMD4Hash &fileHash, uint32 part);

    //! Select exactly one part of one file for download from one specific source (legacy API).
    void SetSource(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash);

    //! Lock one part so it cannot be requested from any source.
    void SetLocked(const CMD4Hash &fileHash, uint32 part);

    //! Begin a default-deny route plan for one file. Parts must then be explicitly
    //! assigned with SetPartSource or unlocked with SetNormal.
    void SetExclusive(const CMD4Hash &fileHash);

    //! Configure one sequential relay stage: retrieve parts before stagePart from upstream,
    //! then retrieve stagePart from origin.
    void SetSequence(const CMD4Hash &fileHash,
        uint32 stagePart,
        const CMD4Hash &upstreamUserHash,
        const CMD4Hash &originUserHash,
        const CMD4Hash &downstreamUserHash,
        uint32 downstreamIP = 0,
        uint16 downstreamPort = 0);

    //! Configure a relay using peer nicknames for discovery. The origin hash is shared by
    //! every node and is checked against each upstream OFFER.
    void SetSequenceByName(const CMD4Hash &fileHash,
        uint32 stagePart,
        const CMD4Hash &originUserHash,
        const wxString &upstreamName,
        const wxString &downstreamName);
    //! Configure a relay from one shared nickname base; suffixes identify sequence steps.
    void SetSequenceByBase(const CMD4Hash &fileHash,
        uint32 stagePart,
        const CMD4Hash &originUserHash,
        const wxString &nicknameBase,
        bool pingPong = false,
        bool finalPartFirst = false);
    //! Persist/restore nickname-based relay configuration across process restarts.
    void SavePersistentSequence();
    bool RestorePersistentSequence();
    const wxString &GetSequenceNicknameBase();
    bool ValidateSequenceLocalNickname(const CMD4Hash &fileHash, const wxString &nickname);
    void MarkSequenceFinalStage(const CMD4Hash &fileHash);
    //! Report a relay state transition to the daemon terminal and GUI notification area.
    void ReportSequenceProgress(const wxString &status);
    bool BindSequenceUpstream(
        const CMD4Hash &fileHash, const CMD4Hash &userHash, uint32 ip = 0, uint16 port = 0);
    bool BindSequenceOrigin(const CMD4Hash &fileHash, const CMD4Hash &userHash);
    bool BindSequenceDownstream(const CMD4Hash &fileHash,
        const CMD4Hash &userHash,
        uint32 ip = 0,
        uint16 port = 0);
    const wxString &GetSequenceUpstreamName();
    const wxString &GetSequenceDownstreamName();
    bool IsSequenceByName();
    bool IsSequencePingPong();
    bool IsSequenceFinalPartFirst();
    //! Map a relay sequence step to the physical part, optionally final-part-first.
    uint32 GetSequenceFilePart(uint32 partCount, uint32 sequenceStep);
    //! Convert a physical part selected in the GUI to a sequence step.
    uint32 GetSequenceStepForFilePart(uint32 partCount, uint32 filePart, bool finalPartFirst);
    //! Initialize a reordered route after the file's part count is known.
    void InitializeSequencePartOrder(const CMD4Hash &fileHash, uint32 partCount);
    bool IsSequencePingPongPeer(const CMD4Hash &fileHash, const CMD4Hash &userHash);
    const CMD4Hash &GetSequenceBackfillPeer();
    uint32 GetSequenceBackfillPeerIP();
    uint16 GetSequenceBackfillPeerPort();
    bool AdvanceSequencePingPong(const CMD4Hash &fileHash, uint32 peerPart, uint32 partCount);
    bool CanProbeSequenceUpstream(const CMD4Hash &fileHash, const CMD4Hash &userHash);
    bool SetSequenceUpstreamCandidate(
        const CMD4Hash &fileHash, const CMD4Hash &userHash, uint32 ip, uint16 port);
    bool ShouldSendSequenceJoin(const CMD4Hash &fileHash, uint64 now);
    uint32 GetSequenceUpstreamCandidateIP();
    uint16 GetSequenceUpstreamCandidatePort();

    //! Return the pending relay stage, if the prefix is still being fetched.
    bool IsSequenceConfigured(const CMD4Hash &fileHash);
    bool IsSequenceWaitingForPrefix(const CMD4Hash &fileHash);
    bool IsSequenceOriginStarted(const CMD4Hash &fileHash);
    bool IsSequenceUpstreamConfirmed();
    bool IsSequenceStageFinished(const CMD4Hash &fileHash);
    uint32 GetSequencePart();
    const CMD4Hash &GetSequenceUpstream();
    const CMD4Hash &GetSequenceUpstreamCandidate();
    const CMD4Hash &GetSequenceOrigin();
    const CMD4Hash &GetSequenceDownstream();
    uint32 GetSequenceDownstreamIP();
    uint16 GetSequenceDownstreamPort();

    //! Enable the origin route after the prefix has been verified.
    void StartSequenceOrigin(const CMD4Hash &fileHash);
    void FinishSequencePart(const CMD4Hash &fileHash);
    bool ShouldSendSequenceAck(const CMD4Hash &fileHash, uint64 now);
    void MarkSequenceAckQueued(const CMD4Hash &fileHash);
    bool ShouldSendSequenceBackfill(const CMD4Hash &fileHash, uint32 part, uint64 now);
    bool MarkSequenceBackfillAcknowledged(const CMD4Hash &fileHash, uint32 part);
    bool IsSequenceBackfillAcknowledged(const CMD4Hash &fileHash, uint32 part);
    bool AcceptSequenceBackfill(const CMD4Hash &fileHash, uint32 part);
    bool IsSequenceBackfillReceived(const CMD4Hash &fileHash, uint32 part);
    bool ShouldAcknowledgeSequenceBackfill(const CMD4Hash &fileHash, uint32 part, uint64 now);
    void MarkSequenceBackfillAcknowledgedLocally(const CMD4Hash &fileHash, uint32 part);
    bool ShouldSendSequenceOffer(const CMD4Hash &fileHash, uint64 now);
    void MarkSequenceDownstreamAcknowledged(const CMD4Hash &fileHash);
    bool IsSequenceDownstreamAcknowledged(const CMD4Hash &fileHash);

    //! Select one part from one specific source.
    void SetPartSource(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash);

    //! Remove the rule for one part, returning it to normal selection.
    void SetNormal(const CMD4Hash &fileHash, uint32 part);

    //! Disable all forced-part restrictions.
    void Clear();

    //! Return whether any forced restriction is currently active.
    bool IsActive();

    //! Return whether the active legacy restriction is tied to one source.
    bool IsSourceRestricted();

    //! Return the file hash targeted by the active restriction.
    const CMD4Hash &GetFileHash();

    //! Return the selected part of the active legacy restriction.
    uint32 GetPart();

    //! Return the UserHash targeted by the active legacy source restriction.
    const CMD4Hash &GetSourceUserHash();

    //! Return true when the given file/part is allowed to be requested.
    bool IsAllowed(const CMD4Hash &fileHash, uint32 part);

    //! Return true when the given file/part/source combination is allowed to be requested.
    bool IsSourceAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash);

    //! Return true when a source may provide the given part to this node.
    bool IsDownloadAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash);

    //! Return true when this node may upload the given part to the requesting peer.
    bool IsUploadAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &peerUserHash);

    //! Return true if at least one part of the file may be requested from this source.
    bool CanRequestFromSource(const CMD4Hash &fileHash, const CMD4Hash &userHash);

} // namespace ForcePartSelection

#endif // FORCE_PART_SELECTION_H
