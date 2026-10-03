//
// This file is part of the aMule Project.
//
// Temporary command-line restriction for downloading one part of one file.
//

#include "ForcePartSelection.h"

#include <map>
#include <wx/config.h>

#include <common/Format.h>
#include "Logger.h"
#ifndef AMULE_DAEMON
#include <wx/app.h>
#include <wx/notifmsg.h>
#endif

namespace {
enum PartRuleType {
	RULE_NORMAL,
	RULE_LOCKED,
	RULE_SOURCE
};

struct PartRule {
	PartRuleType type;
	CMD4Hash userHash;

	PartRule() : type(RULE_NORMAL) {}
	explicit PartRule(PartRuleType ruleType) : type(ruleType) {}
	PartRule(PartRuleType ruleType, const CMD4Hash &hash)
		: type(ruleType), userHash(hash) {}
};

CMD4Hash s_fileHash;
std::map<uint32, PartRule> s_rules;
bool s_active = false;
bool s_default_locked = false;
bool s_sequence_waiting_for_prefix = false;
bool s_sequence_active = false;
uint32 s_sequence_part = 0;
CMD4Hash s_sequence_upstream;
CMD4Hash s_sequence_upstream_candidate;
uint32 s_sequence_upstream_candidate_ip = 0;
uint16 s_sequence_upstream_candidate_port = 0;
CMD4Hash s_sequence_origin;
CMD4Hash s_sequence_downstream;
wxString s_sequence_upstream_name;
wxString s_sequence_downstream_name;
wxString s_sequence_nickname_base;
bool s_sequence_by_name = false;
bool s_sequence_by_base = false;
bool s_sequence_pingpong = false;
bool s_sequence_final_part_first = false;
bool s_sequence_part_order_initialized = false;
uint32 s_sequence_part_count = 0;
bool s_sequence_upstream_confirmed = false;
uint32 s_sequence_downstream_ip = 0;
uint16 s_sequence_downstream_port = 0;
uint64 s_sequence_last_ack_tick = 0;
bool s_sequence_ack_queued = false;
uint64 s_sequence_last_backfill_tick = 0;
uint32 s_sequence_backfill_part = 0;
uint32 s_sequence_backfill_acked_part = 0;
uint32 s_sequence_backfill_received_part = 0;
uint32 s_sequence_backfill_ack_sent_part = 0;
uint64 s_sequence_last_backfill_ack_tick = 0;
uint32 s_sequence_backfill_ack_attempt_part = 0;
uint64 s_sequence_last_offer_tick = 0;
uint64 s_sequence_last_join_tick = 0;
bool s_sequence_downstream_acknowledged = false;
bool s_sequence_stage_finished = false;
bool s_sequence_backfill_acked_any = false;
bool s_sequence_initial_backfill_received = false;
const uint64 SEQUENCE_MESSAGE_RETRY_MS = 30000;

// Compatibility state for the original "one part only" API.
bool s_legacy_single_part = false;
uint32 s_part = 0;
CMD4Hash s_userHash;
bool s_source_active = false;

const PartRule *FindRule(const CMD4Hash &fileHash, uint32 part)
{
	if (!s_active || fileHash != s_fileHash) {
		return nullptr;
	}

	std::map<uint32, PartRule>::const_iterator it = s_rules.find(part);
	return it == s_rules.end() ? nullptr : &it->second;
}
} // namespace

namespace ForcePartSelection {

void Set(const CMD4Hash &fileHash, uint32 part)
{
	Clear();
	s_fileHash = fileHash;
	s_part = part;
	s_active = true;
	s_legacy_single_part = true;
	s_source_active = false;
}

void SetSource(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash)
{
	Clear();
	s_fileHash = fileHash;
	s_userHash = userHash;
	s_part = part;
	s_active = true;
	s_legacy_single_part = true;
	s_source_active = true;
}

void SetLocked(const CMD4Hash &fileHash, uint32 part)
{
	if (!s_active || fileHash != s_fileHash) {
		Clear();
		s_fileHash = fileHash;
		s_active = true;
	}

	s_legacy_single_part = false;
	s_rules[part] = PartRule(RULE_LOCKED);
}

void SetExclusive(const CMD4Hash &fileHash)
{
	Clear();
	s_fileHash = fileHash;
	s_active = true;
	s_legacy_single_part = false;
	s_default_locked = true;
}

void SetSequence(const CMD4Hash &fileHash,
	uint32 stagePart,
	const CMD4Hash &upstreamUserHash,
	const CMD4Hash &originUserHash,
	const CMD4Hash &downstreamUserHash,
	uint32 downstreamIP,
	uint16 downstreamPort)
{
	SetExclusive(fileHash);
	s_sequence_active = true;
	s_sequence_part = stagePart;
	s_sequence_upstream = upstreamUserHash;
	s_sequence_origin = originUserHash;
	s_sequence_downstream = downstreamUserHash;
	s_sequence_downstream_ip = downstreamIP;
	s_sequence_downstream_port = downstreamPort;
	s_sequence_last_ack_tick = 0;
	s_sequence_ack_queued = false;
	s_sequence_last_backfill_tick = 0;
	s_sequence_backfill_part = stagePart;
	s_sequence_backfill_acked_part = stagePart ? stagePart - 1 : 0;
	s_sequence_backfill_received_part = stagePart;
	s_sequence_backfill_ack_sent_part = stagePart;
	s_sequence_last_backfill_ack_tick = 0;
	s_sequence_backfill_ack_attempt_part = 0;
	s_sequence_last_offer_tick = 0;
	s_sequence_last_join_tick = 0;
	s_sequence_downstream_acknowledged = false;
	s_sequence_stage_finished = false;
	s_sequence_part_order_initialized = false;
	s_sequence_part_count = 0;
	for (uint32 part = 0; part < stagePart; ++part) {
		SetPartSource(fileHash, part, upstreamUserHash);
	}
	if (stagePart == 0) {
		SetPartSource(fileHash, stagePart, originUserHash);
	} else {
		s_sequence_waiting_for_prefix = true;
	}
}

void SetSequenceByName(const CMD4Hash &fileHash,
	uint32 stagePart,
	const CMD4Hash &originUserHash,
	const wxString &upstreamName,
	const wxString &downstreamName)
{
	SetExclusive(fileHash);
	s_sequence_active = true;
	s_sequence_by_name = true;
	s_sequence_pingpong = false;
	s_sequence_final_part_first = false;
	s_sequence_upstream_confirmed = false;
	s_sequence_part = stagePart;
	s_sequence_origin = originUserHash;
	s_sequence_upstream_name = upstreamName;
	s_sequence_downstream_name = downstreamName;
	s_sequence_last_ack_tick = 0;
	s_sequence_ack_queued = false;
	s_sequence_last_backfill_tick = 0;
	s_sequence_backfill_part = stagePart;
	s_sequence_backfill_acked_part = stagePart ? stagePart - 1 : 0;
	s_sequence_backfill_received_part = stagePart;
	s_sequence_backfill_ack_sent_part = stagePart;
	s_sequence_last_backfill_ack_tick = 0;
	s_sequence_backfill_ack_attempt_part = 0;
	s_sequence_last_offer_tick = 0;
	s_sequence_last_join_tick = 0;
	s_sequence_downstream_acknowledged = false;
	s_sequence_stage_finished = false;
	s_sequence_part_order_initialized = false;
	s_sequence_part_count = 0;
	if (stagePart == 0) {
		if (!originUserHash.IsEmpty()) {
			SetPartSource(fileHash, stagePart, originUserHash);
		}
	} else {
		s_sequence_waiting_for_prefix = true;
	}
}

void SetSequenceByBase(const CMD4Hash &fileHash,
	uint32 stagePart,
	const CMD4Hash &originUserHash,
	const wxString &nicknameBase,
	bool pingPong,
	bool finalPartFirst)
{
	const uint32 nodeIndex = pingPong ? stagePart % 2 : stagePart;
	const uint32 peerIndex = pingPong ? 1 - nodeIndex : nodeIndex + 1;
	const wxString peerName = nicknameBase + wxString::Format("-%u", peerIndex);
	const wxString upstreamName = stagePart == 0 ? wxString() :
		(pingPong ? peerName : nicknameBase + wxString::Format("-%u", stagePart - 1));
	const wxString downstreamName = pingPong ? peerName :
		nicknameBase + wxString::Format("-%u", stagePart + 1);
	SetSequenceByName(fileHash, stagePart, originUserHash, upstreamName, downstreamName);
	s_sequence_nickname_base = nicknameBase;
	s_sequence_by_base = true;
	s_sequence_pingpong = pingPong;
	s_sequence_final_part_first = finalPartFirst;
	if (pingPong || finalPartFirst) {
		s_sequence_part_order_initialized = false;
		s_sequence_part_count = 0;
		s_rules.clear();
	}
	if (pingPong) {
		s_sequence_backfill_part = stagePart;
		s_sequence_backfill_acked_part = s_sequence_backfill_part
			? s_sequence_backfill_part - 1 : 0;
		s_sequence_backfill_acked_any = false;
		s_sequence_initial_backfill_received = false;
		if (stagePart == 0) {
			s_sequence_waiting_for_prefix = false;
			s_rules[stagePart] = PartRule(RULE_SOURCE, s_sequence_origin);
		}
	}
	SavePersistentSequence();
}

void SavePersistentSequence()
{
	wxConfigBase *config = wxConfigBase::Get();
	if (!config) {
		return;
	}
	if (!s_active || !s_sequence_active || !s_sequence_by_base) {
		config->DeleteGroup("/KeroroRelay");
		config->Flush();
		return;
	}
	config->Write("/KeroroRelay/Enabled", true);
	config->Write("/KeroroRelay/FileHash", s_fileHash.Encode());
	config->Write("/KeroroRelay/Part", (long)s_sequence_part);
	config->Write("/KeroroRelay/OriginUserHash", s_sequence_origin.Encode());
	config->Write("/KeroroRelay/NicknameBase", s_sequence_nickname_base);
	config->Write("/KeroroRelay/PingPong", s_sequence_pingpong);
	config->Write("/KeroroRelay/FinalPartFirst", s_sequence_final_part_first);
	config->Write("/KeroroRelay/PartOrderVersion", (long)2);
	config->Flush();
}

bool RestorePersistentSequence()
{
	wxConfigBase *config = wxConfigBase::Get();
	if (!config) {
		return false;
	}
	bool enabled = false;
	config->Read("/KeroroRelay/Enabled", &enabled, false);
	if (!enabled) {
		return false;
	}
	wxString fileHashText;
	wxString originHashText;
	wxString nicknameBase;
	long part = -1;
	long partOrderVersion = 0;
	bool pingPong = false;
	bool finalPartFirst = false;
	config->Read("/KeroroRelay/FileHash", &fileHashText);
	config->Read("/KeroroRelay/OriginUserHash", &originHashText);
	config->Read("/KeroroRelay/NicknameBase", &nicknameBase);
	config->Read("/KeroroRelay/Part", &part, (long)-1);
	config->Read("/KeroroRelay/PingPong", &pingPong, false);
	config->Read("/KeroroRelay/FinalPartFirst", &finalPartFirst, false);
	config->Read("/KeroroRelay/PartOrderVersion", &partOrderVersion, (long)0);
	if (pingPong && partOrderVersion == 1) {
		// Version 1 ping-pong state used the final-first schedule by default.
		finalPartFirst = true;
	} else if ((pingPong || finalPartFirst) && partOrderVersion != 2) {
		// The reordered route changed; discard incompatible persisted stage state.
		config->DeleteGroup("/KeroroRelay");
		config->Flush();
		return false;
	}
	CMD4Hash fileHash;
	CMD4Hash originHash;
	if (!fileHash.Decode(fileHashText) || !originHash.Decode(originHashText) ||
		part < 0 || part >= 0xffff || nicknameBase.IsEmpty()) {
		config->DeleteGroup("/KeroroRelay");
		config->Flush();
		return false;
	}
	SetSequenceByBase(fileHash, static_cast<uint32>(part), originHash, nicknameBase,
		pingPong, finalPartFirst);
	return true;
}

const wxString &GetSequenceNicknameBase()
{
	return s_sequence_nickname_base;
}

bool ValidateSequenceLocalNickname(const CMD4Hash &fileHash, const wxString &nickname)
{
	if (!IsSequenceConfigured(fileHash) || !s_sequence_by_base) {
		return true;
	}
	const uint32 nodeIndex = s_sequence_pingpong ? s_sequence_part % 2 : s_sequence_part;
	const wxString expected = s_sequence_nickname_base + wxString::Format("-%u", nodeIndex);
	if (nickname == expected) {
		return true;
	}
	return false;
}

uint32 GetSequenceFilePart(uint32 partCount, uint32 sequenceStep)
{
	if (!s_sequence_final_part_first || partCount == 0 || sequenceStep >= partCount) {
		return sequenceStep;
	}
	return sequenceStep == 0 ? partCount - 1 : sequenceStep - 1;
}

uint32 GetSequenceStepForFilePart(uint32 partCount, uint32 filePart, bool finalPartFirst)
{
	if (!finalPartFirst || partCount == 0 || filePart >= partCount) {
		return filePart;
	}
	return filePart == partCount - 1 ? 0 : filePart + 1;
}

void InitializeSequencePartOrder(const CMD4Hash &fileHash, uint32 partCount)
{
	if (!IsSequenceConfigured(fileHash) ||
		(!s_sequence_pingpong && !s_sequence_final_part_first) ||
		s_sequence_part_order_initialized || partCount == 0) {
		return;
	}
	s_sequence_part_order_initialized = true;
	s_sequence_part_count = partCount;
	s_rules.clear();
	if (s_sequence_part >= partCount) {
		return;
	}
	if (s_sequence_part == 0) {
		s_rules[GetSequenceFilePart(partCount, 0)] = PartRule(RULE_SOURCE, s_sequence_origin);
	} else if (s_sequence_pingpong && s_sequence_part > 1 && !s_sequence_origin.IsEmpty()) {
		s_rules[GetSequenceFilePart(partCount, s_sequence_part)] =
			PartRule(RULE_SOURCE, s_sequence_origin);
	} else {
		s_sequence_waiting_for_prefix = true;
	}
}

void MarkSequenceFinalStage(const CMD4Hash &fileHash)
{
	if (IsSequenceConfigured(fileHash) && s_sequence_by_base && !s_sequence_pingpong) {
		s_sequence_downstream_name.clear();
		s_sequence_downstream.Clear();
	}
}

void ReportSequenceProgress(const wxString &status)
{
	AddLogLineNS(CFormat("[Relay] %s\n") % status);
#ifndef AMULE_DAEMON
	if (wxTheApp && wxTheApp->GetTopWindow()) {
		wxNotificationMessage *notification = new wxNotificationMessage(
			_("Relay progress"), status, wxTheApp->GetTopWindow());
		notification->SetFlags(wxICON_WARNING);
		notification->Show(8);
		delete notification;
	}
#endif
}

bool BindSequenceUpstream(
	const CMD4Hash &fileHash, const CMD4Hash &userHash, uint32 ip, uint16 port)
{
	if (!IsSequenceConfigured(fileHash) || !s_sequence_by_name || userHash.IsEmpty() ||
		(s_sequence_part == 0) ||
		(!s_sequence_upstream.IsEmpty() && s_sequence_upstream != userHash) ||
		(!s_sequence_upstream_candidate.IsEmpty() && s_sequence_upstream_candidate != userHash)) {
		return false;
	}
	s_sequence_upstream = userHash;
	s_sequence_upstream_candidate = userHash;
	if (ip != 0) {
		s_sequence_upstream_candidate_ip = ip;
		s_sequence_upstream_candidate_port = port;
	}
	s_sequence_upstream_confirmed = true;
	for (uint32 step = 0; step < s_sequence_part; ++step) {
		const uint32 part = GetSequenceFilePart(s_sequence_part_count, step);
		s_rules[part] = PartRule(RULE_SOURCE, userHash);
	}
	return true;
}

bool BindSequenceOrigin(const CMD4Hash &fileHash, const CMD4Hash &userHash)
{
	if (!IsSequenceConfigured(fileHash) || !s_sequence_by_name || userHash.IsEmpty() ||
		(!s_sequence_origin.IsEmpty() && s_sequence_origin != userHash)) {
		return false;
	}
	s_sequence_origin = userHash;
	if ((s_sequence_part == 0 || !s_sequence_waiting_for_prefix) &&
		!s_sequence_stage_finished) {
		s_rules[GetSequenceFilePart(s_sequence_part_count, s_sequence_part)] =
			PartRule(RULE_SOURCE, userHash);
	}
	return true;
}

bool BindSequenceDownstream(
	const CMD4Hash &fileHash, const CMD4Hash &userHash, uint32 ip, uint16 port)
{
	if (!IsSequenceConfigured(fileHash) || !s_sequence_by_name || userHash.IsEmpty() ||
		s_sequence_downstream_name.IsEmpty() ||
		(!s_sequence_downstream.IsEmpty() && s_sequence_downstream != userHash)) {
		return false;
	}
	s_sequence_downstream = userHash;
	s_sequence_downstream_ip = ip;
	s_sequence_downstream_port = port;
	return true;
}

const wxString &GetSequenceUpstreamName()
{
	return s_sequence_upstream_name;
}

const wxString &GetSequenceDownstreamName()
{
	return s_sequence_downstream_name;
}

bool IsSequenceByName()
{
	return s_sequence_by_name;
}

bool IsSequencePingPong()
{
	return s_sequence_pingpong;
}

bool IsSequenceFinalPartFirst()
{
	return s_sequence_final_part_first;
}

bool IsSequencePingPongPeer(const CMD4Hash &fileHash, const CMD4Hash &userHash)
{
	return IsSequenceConfigured(fileHash) && s_sequence_pingpong &&
		!userHash.IsEmpty() && userHash == GetSequenceBackfillPeer();
}

const CMD4Hash &GetSequenceBackfillPeer()
{
	if (!s_sequence_pingpong || !s_sequence_upstream.IsEmpty()) {
		return s_sequence_upstream;
	}
	return s_sequence_downstream;
}

uint32 GetSequenceBackfillPeerIP()
{
	if (!s_sequence_pingpong || !s_sequence_upstream.IsEmpty()) {
		return s_sequence_upstream_candidate_ip;
	}
	return s_sequence_downstream_ip;
}

uint16 GetSequenceBackfillPeerPort()
{
	if (!s_sequence_pingpong || !s_sequence_upstream.IsEmpty()) {
		return s_sequence_upstream_candidate_port;
	}
	return s_sequence_downstream_port;
}

bool AdvanceSequencePingPong(const CMD4Hash &fileHash, uint32 peerPart, uint32 partCount)
{
	const uint32 peerStep = s_sequence_part + 1;
	if (!IsSequenceConfigured(fileHash) || !s_sequence_pingpong ||
		peerStep >= partCount ||
		peerPart != GetSequenceFilePart(partCount, peerStep)) {
		return false;
	}
	s_rules[peerPart] = PartRule(RULE_LOCKED);
	const uint32 nextStep = peerStep + 1;
	if (nextStep >= partCount) {
		// The peer supplied the final opposite-parity part. There is no next
		// origin part to request, but keep our parity identity and mark the
		// relay stage complete.
		s_sequence_backfill_received_part = peerStep;
		s_sequence_stage_finished = true;
		SavePersistentSequence();
		return true;
	}
	s_sequence_part = nextStep;
	const uint32 nextFilePart = GetSequenceFilePart(partCount, nextStep);
	s_rules[nextFilePart] = PartRule(RULE_SOURCE, s_sequence_origin);
	s_sequence_waiting_for_prefix = false;
	s_sequence_stage_finished = false;
	s_sequence_part_order_initialized = false;
	s_sequence_part_count = 0;
	s_sequence_backfill_acked_any = false;
	s_sequence_initial_backfill_received = false;
	s_sequence_last_ack_tick = 0;
	s_sequence_ack_queued = false;
	s_sequence_last_offer_tick = 0;
	s_sequence_backfill_part = nextStep;
	s_sequence_backfill_acked_part = nextStep - 1;
	s_sequence_backfill_acked_any = false;
	s_sequence_last_backfill_tick = 0;
	s_sequence_backfill_received_part = peerStep;
	s_sequence_backfill_ack_sent_part = peerStep;
	s_sequence_last_backfill_ack_tick = 0;
	s_sequence_backfill_ack_attempt_part = 0;
	s_sequence_part_count = partCount;
	SavePersistentSequence();
	return true;
}

bool CanProbeSequenceUpstream(const CMD4Hash &fileHash, const CMD4Hash &userHash)
{
	return IsSequenceConfigured(fileHash) && s_sequence_by_name &&
		(s_sequence_waiting_for_prefix || (s_sequence_pingpong && s_sequence_part > 1)) &&
		s_sequence_part > 0 &&
		(!s_sequence_pingpong || s_sequence_downstream.IsEmpty()) &&
		!s_sequence_upstream_confirmed && !s_sequence_origin.IsEmpty() && !userHash.IsEmpty() &&
		userHash != s_sequence_origin &&
		(s_sequence_upstream_candidate.IsEmpty() || s_sequence_upstream_candidate == userHash);
}

bool SetSequenceUpstreamCandidate(
	const CMD4Hash &fileHash, const CMD4Hash &userHash, uint32 ip, uint16 port)
{
	if (!CanProbeSequenceUpstream(fileHash, userHash) ||
		(!s_sequence_upstream_candidate.IsEmpty() && s_sequence_upstream_candidate != userHash)) {
		return false;
	}
	if (!s_sequence_upstream_candidate.IsEmpty()) {
		return false;
	}
	s_sequence_upstream_candidate = userHash;
	s_sequence_upstream_candidate_ip = ip;
	s_sequence_upstream_candidate_port = port;
	return true;
}

bool ShouldSendSequenceJoin(const CMD4Hash &fileHash, uint64 now)
{
	if (!IsSequenceConfigured(fileHash) || !s_sequence_by_name ||
		s_sequence_upstream_candidate.IsEmpty() || s_sequence_upstream_confirmed ||
		(s_sequence_last_join_tick && now - s_sequence_last_join_tick < SEQUENCE_MESSAGE_RETRY_MS)) {
		return false;
	}
	s_sequence_last_join_tick = now;
	return true;
}

const CMD4Hash &GetSequenceUpstreamCandidate()
{
	return s_sequence_upstream_candidate;
}

uint32 GetSequenceUpstreamCandidateIP()
{
	return s_sequence_upstream_candidate_ip;
}

uint16 GetSequenceUpstreamCandidatePort()
{
	return s_sequence_upstream_candidate_port;
}

bool IsSequenceWaitingForPrefix(const CMD4Hash &fileHash)
{
	return s_active && fileHash == s_fileHash && s_sequence_waiting_for_prefix;
}

bool IsSequenceConfigured(const CMD4Hash &fileHash)
{
	return s_active && fileHash == s_fileHash && s_sequence_active;
}

bool IsSequenceOriginStarted(const CMD4Hash &fileHash)
{
	return IsSequenceConfigured(fileHash) && !s_sequence_waiting_for_prefix;
}

bool IsSequenceUpstreamConfirmed()
{
	return !s_sequence_by_name || s_sequence_upstream_confirmed;
}

bool IsSequenceStageFinished(const CMD4Hash &fileHash)
{
	return IsSequenceConfigured(fileHash) && s_sequence_stage_finished;
}

uint32 GetSequencePart()
{
	return s_sequence_part;
}

const CMD4Hash &GetSequenceUpstream()
{
	return s_sequence_upstream;
}

const CMD4Hash &GetSequenceOrigin()
{
	return s_sequence_origin;
}

const CMD4Hash &GetSequenceDownstream()
{
	return s_sequence_downstream;
}

uint32 GetSequenceDownstreamIP()
{
	return s_sequence_downstream_ip;
}

uint16 GetSequenceDownstreamPort()
{
	return s_sequence_downstream_port;
}

void StartSequenceOrigin(const CMD4Hash &fileHash)
{
	if (!IsSequenceWaitingForPrefix(fileHash)) {
		return;
	}
	for (uint32 step = 0; step < s_sequence_part; ++step) {
		s_rules[GetSequenceFilePart(s_sequence_part_count, step)] = PartRule(RULE_LOCKED);
	}
	s_rules[GetSequenceFilePart(s_sequence_part_count, s_sequence_part)] =
		PartRule(RULE_SOURCE, s_sequence_origin);
	s_sequence_waiting_for_prefix = false;
}

void FinishSequencePart(const CMD4Hash &fileHash)
{
	if (!IsSequenceConfigured(fileHash) || s_sequence_stage_finished) {
		return;
	}
	s_rules[GetSequenceFilePart(s_sequence_part_count, s_sequence_part)] = PartRule(RULE_LOCKED);
	s_sequence_stage_finished = true;
}

bool ShouldSendSequenceAck(const CMD4Hash &fileHash, uint64 now)
{
	if (!IsSequenceOriginStarted(fileHash) || s_sequence_part == 0 ||
		s_sequence_upstream.IsEmpty() || s_sequence_ack_queued ||
		(s_sequence_last_ack_tick && now - s_sequence_last_ack_tick < SEQUENCE_MESSAGE_RETRY_MS)) {
		return false;
	}
	s_sequence_last_ack_tick = now;
	return true;
}

void MarkSequenceAckQueued(const CMD4Hash &fileHash)
{
	if (IsSequenceConfigured(fileHash) && s_sequence_part > 0) {
		s_sequence_ack_queued = true;
	}
}

bool ShouldSendSequenceBackfill(const CMD4Hash &fileHash, uint32 part, uint64 now)
{
	if (!IsSequenceConfigured(fileHash) || (s_sequence_part == 0 && !s_sequence_pingpong) ||
		GetSequenceBackfillPeer().IsEmpty() || part != s_sequence_backfill_part ||
		(s_sequence_pingpong && part % 2 != s_sequence_part % 2) ||
		(s_sequence_backfill_acked_any && part <= s_sequence_backfill_acked_part) ||
		(s_sequence_last_backfill_tick && now - s_sequence_last_backfill_tick < SEQUENCE_MESSAGE_RETRY_MS)) {
		return false;
	}
	s_sequence_last_backfill_tick = now;
	return true;
}

bool MarkSequenceBackfillAcknowledged(const CMD4Hash &fileHash, uint32 part)
{
	if (!IsSequenceConfigured(fileHash) || part != s_sequence_backfill_part ||
		(s_sequence_backfill_acked_any && part <= s_sequence_backfill_acked_part)) {
		return false;
	}
	s_sequence_backfill_acked_part = part;
	s_sequence_backfill_acked_any = true;
	s_sequence_backfill_part = part + 1;
	s_sequence_last_backfill_tick = 0;
	return true;
}

bool IsSequenceBackfillAcknowledged(const CMD4Hash &fileHash, uint32 part)
{
	return IsSequenceConfigured(fileHash) && s_sequence_backfill_acked_any &&
		s_sequence_backfill_acked_part >= part;
}

bool AcceptSequenceBackfill(const CMD4Hash &fileHash, uint32 part)
{
	if (IsSequenceConfigured(fileHash) && s_sequence_pingpong &&
		s_sequence_part == 1 && part == 0) {
		if (s_sequence_initial_backfill_received) {
			return false;
		}
		s_sequence_initial_backfill_received = true;
		return true;
	}
	if (!IsSequenceConfigured(fileHash) || part <= s_sequence_part) {
		return false;
	}
	const uint32 expectedPart = s_sequence_pingpong
		? s_sequence_part + 1
		: s_sequence_backfill_ack_sent_part + 1;
	if (part > s_sequence_backfill_received_part && part != expectedPart) {
		return false;
	}
	if (part > s_sequence_backfill_received_part) {
		s_sequence_backfill_received_part = part;
		return true;
	}
	return false;
}

bool IsSequenceBackfillReceived(const CMD4Hash &fileHash, uint32 part)
{
	return IsSequenceConfigured(fileHash) &&
		((s_sequence_pingpong && s_sequence_part == 1 && part == 0 &&
			s_sequence_initial_backfill_received) ||
			(part > s_sequence_part && part <= s_sequence_backfill_received_part));
}

bool ShouldAcknowledgeSequenceBackfill(const CMD4Hash &fileHash, uint32 part, uint64 now)
{
	const bool initialPingPongBackfill = s_sequence_pingpong && s_sequence_part == 1 &&
		part == 0 && s_sequence_initial_backfill_received;
	const uint32 expectedPart = s_sequence_pingpong
		? s_sequence_part + 1
		: s_sequence_backfill_ack_sent_part + 1;
	if (!IsSequenceConfigured(fileHash) ||
		(!initialPingPongBackfill && (part != expectedPart ||
			part > s_sequence_backfill_received_part))) {
		return false;
	}
	if (s_sequence_backfill_ack_attempt_part == part && s_sequence_last_backfill_ack_tick &&
		now - s_sequence_last_backfill_ack_tick < SEQUENCE_MESSAGE_RETRY_MS) {
		return false;
	}
	s_sequence_backfill_ack_attempt_part = part;
	s_sequence_last_backfill_ack_tick = now;
	return true;
}

void MarkSequenceBackfillAcknowledgedLocally(const CMD4Hash &fileHash, uint32 part)
{
	const bool initialPingPongBackfill = s_sequence_pingpong && s_sequence_part == 1 &&
		part == 0 && s_sequence_initial_backfill_received;
	const uint32 expectedPart = s_sequence_pingpong
		? s_sequence_part + 1
		: s_sequence_backfill_ack_sent_part + 1;
	if (IsSequenceConfigured(fileHash) &&
		(initialPingPongBackfill || (part == expectedPart &&
			part <= s_sequence_backfill_received_part))) {
		s_sequence_backfill_ack_sent_part = part;
		s_sequence_last_backfill_ack_tick = 0;
		s_sequence_backfill_ack_attempt_part = 0;
	}
}

bool ShouldSendSequenceOffer(const CMD4Hash &fileHash, uint64 now)
{
	if (!IsSequenceConfigured(fileHash) || !s_sequence_stage_finished ||
		s_sequence_downstream.IsEmpty() ||
		s_sequence_downstream_acknowledged ||
		(s_sequence_last_offer_tick && now - s_sequence_last_offer_tick < SEQUENCE_MESSAGE_RETRY_MS)) {
		return false;
	}
	s_sequence_last_offer_tick = now;
	return true;
}

void MarkSequenceDownstreamAcknowledged(const CMD4Hash &fileHash)
{
	if (IsSequenceConfigured(fileHash) && s_sequence_stage_finished &&
		s_sequence_last_offer_tick != 0) {
		s_sequence_downstream_acknowledged = true;
	}
}

bool IsSequenceDownstreamAcknowledged(const CMD4Hash &fileHash)
{
	return IsSequenceConfigured(fileHash) && s_sequence_downstream_acknowledged;
}

void SetPartSource(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash)
{
	if (!s_active || fileHash != s_fileHash) {
		Clear();
		s_fileHash = fileHash;
		s_active = true;
	}

	s_legacy_single_part = false;
	s_rules[part] = PartRule(RULE_SOURCE, userHash);
}

void SetNormal(const CMD4Hash &fileHash, uint32 part)
{
	if (!s_active || fileHash != s_fileHash) {
		return;
	}

	if (s_default_locked) {
		s_rules[part] = PartRule(RULE_NORMAL);
	} else {
		s_rules.erase(part);
	}
	if (s_rules.empty() && !s_legacy_single_part && !s_default_locked) {
		Clear();
	}
}

void Clear()
{
	if (wxConfigBase::Get()) {
		wxConfigBase::Get()->DeleteGroup("/KeroroRelay");
		wxConfigBase::Get()->Flush();
	}
	s_fileHash.Clear();
	s_userHash.Clear();
	s_rules.clear();
	s_default_locked = false;
	s_sequence_waiting_for_prefix = false;
	s_sequence_active = false;
	s_sequence_part = 0;
	s_sequence_upstream.Clear();
	s_sequence_upstream_candidate.Clear();
	s_sequence_upstream_candidate_ip = 0;
	s_sequence_upstream_candidate_port = 0;
	s_sequence_upstream_confirmed = false;
	s_sequence_origin.Clear();
	s_sequence_downstream.Clear();
	s_sequence_upstream_name.clear();
	s_sequence_downstream_name.clear();
	s_sequence_nickname_base.clear();
	s_sequence_by_name = false;
	s_sequence_by_base = false;
	s_sequence_pingpong = false;
	s_sequence_final_part_first = false;
	s_sequence_part_order_initialized = false;
	s_sequence_part_count = 0;
	s_sequence_last_join_tick = 0;
	s_sequence_downstream_ip = 0;
	s_sequence_downstream_port = 0;
	s_sequence_last_ack_tick = 0;
	s_sequence_ack_queued = false;
	s_sequence_last_backfill_tick = 0;
	s_sequence_backfill_part = 0;
	s_sequence_backfill_acked_part = 0;
	s_sequence_backfill_acked_any = false;
	s_sequence_backfill_received_part = 0;
	s_sequence_backfill_ack_sent_part = 0;
	s_sequence_last_backfill_ack_tick = 0;
	s_sequence_backfill_ack_attempt_part = 0;
	s_sequence_last_offer_tick = 0;
	s_sequence_downstream_acknowledged = false;
	s_sequence_stage_finished = false;
	s_sequence_initial_backfill_received = false;
	s_part = 0;
	s_active = false;
	s_legacy_single_part = false;
	s_source_active = false;
}

bool IsActive()
{
	return s_active;
}

bool IsSourceRestricted()
{
	if (!s_active) {
		return false;
	}
	if (s_legacy_single_part) {
		return s_source_active;
	}
	for (std::map<uint32, PartRule>::const_iterator it = s_rules.begin(); it != s_rules.end(); ++it) {
		if (it->second.type == RULE_SOURCE) {
			return true;
		}
	}
	return false;
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
	if (!s_active || fileHash != s_fileHash) {
		return true;
	}

	if (s_legacy_single_part) {
		return part == s_part;
	}

	const PartRule *rule = FindRule(fileHash, part);
	if (rule == nullptr) {
		return !s_default_locked;
	}

	return rule->type != RULE_LOCKED;
}

bool IsSourceAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash)
{
	if (!s_active || fileHash != s_fileHash) {
		return true;
	}

	if (s_legacy_single_part) {
		if (!s_source_active) {
			return true;
		}

		return part == s_part && userHash == s_userHash;
	}

	const PartRule *rule = FindRule(fileHash, part);
	if (rule == nullptr) {
		return !s_default_locked;
	}

	if (rule->type == RULE_LOCKED) {
		return false;
	}

	if (rule->type == RULE_SOURCE) {
		return userHash == rule->userHash;
	}

	return true;
}

bool IsDownloadAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &userHash)
{
	return IsAllowed(fileHash, part) && IsSourceAllowed(fileHash, part, userHash);
}

bool IsUploadAllowed(const CMD4Hash &fileHash, uint32 part, const CMD4Hash &peerUserHash)
{
	if (!s_active || fileHash != s_fileHash) {
		return true;
	}
	if (!s_sequence_active) {
		return IsAllowed(fileHash, part) && IsSourceAllowed(fileHash, part, peerUserHash);
	}
	if (!s_sequence_stage_finished) {
		return false;
	}
	if (s_sequence_pingpong || s_sequence_final_part_first) {
		bool partWasScheduled = false;
		for (uint32 step = 0; step <= s_sequence_part && step < s_sequence_part_count; ++step) {
			if (GetSequenceFilePart(s_sequence_part_count, step) == part) {
				partWasScheduled = true;
				break;
			}
		}
		if (!partWasScheduled) {
			return false;
		}
	} else if (part > s_sequence_part) {
		return false;
	}
	if (!s_sequence_downstream.IsEmpty()) {
		return peerUserHash == s_sequence_downstream;
	}
	// A stage without a configured successor is the terminal node: share its verified
	// prefix and current part with the network once the complete file has arrived.
	return true;
}

bool CanRequestFromSource(const CMD4Hash &fileHash, const CMD4Hash &userHash)
{
	if (!s_active || fileHash != s_fileHash) {
		return true;
	}

	if (s_legacy_single_part) {
		return !s_source_active || userHash == s_userHash;
	}

	if (!s_default_locked) {
		// Parts without an explicit rule remain available to every source.
		return true;
	}

	for (std::map<uint32, PartRule>::const_iterator it = s_rules.begin(); it != s_rules.end(); ++it) {
		if (it->second.type == RULE_NORMAL ||
			(it->second.type == RULE_SOURCE && it->second.userHash == userHash)) {
			return true;
		}
	}
	return false;
}

} // namespace ForcePartSelection
